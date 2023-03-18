/*****************************************************************************\
 *                                                                           *\
 *  This file is part of the Verificarlo project,                            *\
 *  under the Apache License v2.0 with LLVM Exceptions.                      *\
 *  SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception.                 *\
 *  See https://llvm.org/LICENSE.txt for license information.                *\
 *                                                                           *\
 *                                                                           *\
 *  Copyright (c) 2015                                                       *\
 *     Universite de Versailles St-Quentin-en-Yvelines                       *\
 *     CMLA, Ecole Normale Superieure de Cachan                              *\
 *                                                                           *\
 *  Copyright (c) 2018                                                       *\
 *     Universite de Versailles St-Quentin-en-Yvelines                       *\
 *                                                                           *\
 *  Copyright (c) 2019-2023                                                  *\
 *     Verificarlo Contributors                                              *\
 *                                                                           *\
 ****************************************************************************/
// Changelog:
//
// 2015-05-20 replace random number generator with TinyMT64. This
// provides a reentrant, independent generator of better quality than
// the one provided in libc.
//
// 2015-10-11 New version based on quad floating point type to replace MPFR
// until required MCA precision is lower than quad mantissa divided by 2,
// i.e. 56 bits
//
// 2015-11-16 New version using double precision for single precision operation
//
// 2016-07-14 Support denormalized numbers
//
// 2017-04-25 Rewrite debug and validate the noise addition operation
//
// 2019-08-07 Fix memory leak and convert to interflop
//
// 2020-02-07 create separated virtual precisions for binary32
// and binary64. Uses the binary128 structure for easily manipulating bits
// through bitfields. Removes useless specials cases in qnoise and pow2d.
// Change return type from int to void for some functions and uses instead
// errx and warnx for handling errors.
//
// 2020-02-26 Factorize _inexact function into the _INEXACT macro function.
// Use variables for options name instead of hardcoded one.
// Add DAZ/FTZ support.
//
// 2021-10-13 Switched random number generator from TinyMT64 to the one
// provided by the libc. The backend is now re-entrant. Pthread and OpenMP
// threads are now supported.
// Generation of hook functions is now done through macros, shared accross
// backends.
//
// 2022-02-16 Add interflop_user_call implementation for INTERFLOP_CALL_INEXACT
// id Add FAST_INEXACT macro that is a fast version of the INEXACT macro that
// does not check if the number is representable or not and thus always
// introduces a perturbation (for relativeError only)

#include <argp.h>
#include <err.h>
#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <unistd.h>

#include "interflop/common/float_const.h"
#include "interflop/common/float_struct.h"
#include "interflop/common/float_utils.h"
#include "interflop/common/options.h"
#include "interflop/fma/interflop_fma.h"
#include "interflop/interflop.h"
#include "interflop/iostream/logger.h"
#include "interflop/rng/vfc_rng.h"
#include "interflop_mca.h"

/* Disable thread safety for RNG required for Valgrind */
#ifdef RNG_THREAD_SAFE
#define TLS __thread
#else
#define TLS
#endif

static const char backend_name[] = "interflop-mcaquad";
static const char backend_version[] = "1.x-dev";

typedef enum {
  KEY_PREC_B32,
  KEY_PREC_B64,
  KEY_ERR_EXP,
  KEY_MODE = 'm',
  KEY_ERR_MODE = 'e',
  KEY_SEED = 's',
  KEY_DAZ = 'd',
  KEY_FTZ = 'f',
  KEY_SPARSITY = 'n'
} key_args;

static const char key_prec_b32_str[] = "precision-binary32";
static const char key_prec_b64_str[] = "precision-binary64";
static const char key_mode_str[] = "mode";
static const char key_err_mode_str[] = "error-mode";
static const char key_err_exp_str[] = "max-abs-error-exponent";
static const char key_seed_str[] = "seed";
static const char key_daz_str[] = "daz";
static const char key_ftz_str[] = "ftz";
static const char key_sparsity_str[] = "sparsity";

static const char *MCAQUAD_MODE_STR[] = {[mcaquad_mode_ieee] = "ieee",
                                         [mcaquad_mode_mca] = "mca",
                                         [mcaquad_mode_pb] = "pb",
                                         [mcaquad_mode_rr] = "rr"};

static const char *MCAQUAD_ERR_MODE_STR[] = {[mcaquad_err_mode_rel] = "rel",
                                             [mcaquad_err_mode_abs] = "abs",
                                             [mcaquad_err_mode_all] = "all"};

/* possible operations values */
typedef enum {
  mcaquad_add = '+',
  mcaquad_sub = '-',
  mcaquad_mul = '*',
  mcaquad_div = '/',
  mcaquad_fma = 'f',
  mcaquad_cast = 'c',
  __mcaquad_operations_end__
} mca_operations;

/******************** MCA CONTROL FUNCTIONS *******************
 * The following functions are used to set virtual precision and
 * MCA mode of operation.
 ***************************************************************/

/* Set the mca mode */
static void _set_mcaquad_mode(const mcaquad_mode mode, mcaquad_context_t *ctx) {
  if (mode >= _mcaquad_mode_end_) {
    logger_error("--%s invalid value provided, must be one of: "
                 "{ieee, mca, pb, rr}.",
                 key_mode_str);
  }
  ctx->mode = mode;
}

/* Set the virtual precision for binary32 */
static void _set_mcaquad_precision_binary32(const int precision,
                                            mcaquad_context_t *ctx) {
  _set_precision(MCAQUAD, precision, ctx->binary32_precision, (float)0);
}

/* Set the virtual precision for binary64 */
static void _set_mcaquad_precision_binary64(const int precision,
                                            mcaquad_context_t *ctx) {
  _set_precision(MCAQUAD, precision, ctx->binary64_precision, (double)0);
}

/* Set the error mode */
static void _set_mcaquad_error_mode(mcaquad_err_mode mode,
                                    mcaquad_context_t *ctx) {
  if (mode >= _mcaquad_err_mode_end_) {
    logger_error("invalid error mode provided, must be one of: "
                 "{rel, abs, all}.");
  } else {
    switch (mode) {
    case mcaquad_err_mode_rel:
      ctx->relErr = true;
      ctx->absErr = false;
      break;
    case mcaquad_err_mode_abs:
      ctx->relErr = false;
      ctx->absErr = true;
      break;
    case mcaquad_err_mode_all:
      ctx->relErr = true;
      ctx->absErr = true;
    default:
      break;
    }
  }
}

/* Set the maximal absolute error exponent */
static void _set_mcaquad_max_abs_err_exp(long exponent,
                                         mcaquad_context_t *ctx) {
  ctx->absErr_exp = exponent;
}

/* Set Denormals-Are-Zero flag */
static void _set_mcaquad_daz(bool daz, mcaquad_context_t *ctx) {
  ctx->daz = daz;
}

/* Set Flush-To-Zero flag */
static void _set_mcaquad_ftz(bool ftz, mcaquad_context_t *ctx) {
  ctx->ftz = ftz;
}

/* Set sparsity value */
static void _set_mcaquad_sparsity(float sparsity, mcaquad_context_t *ctx) {
  if (sparsity <= 0) {
    logger_error("invalid value for sparsity %d, must be positive");
  } else {
    ctx->sparsity = sparsity;
  }
}

/* Set RNG seed */
static void _set_mcaquad_seed(uint64_t seed, mcaquad_context_t *ctx) {
  ctx->choose_seed = true;
  ctx->seed = seed;
}

const char *get_mcaquad_mode_name(mcaquad_mode mode) {
  if (mode >= _mcaquad_mode_end_) {
    return NULL;
  } else {
    return MCAQUAD_MODE_STR[mode];
  }
}

const char *INTERFLOP_MCAQUAD_API(get_backend_name)(void) {
  return backend_name;
}

const char *INTERFLOP_MCAQUAD_API(get_backend_version)(void) {
  return backend_version;
}

/******************** MCA RANDOM FUNCTIONS ********************
 * The following functions are used to calculate the random
 * perturbations used for MCA
 ***************************************************************/

/* global thread identifier */
pid_t mcaquad_global_tid = 0;

/* helper data structure to centralize the data used for random number
 * generation */
static TLS rng_state_t rng_state;
/* copy */
static TLS rng_state_t __rng_state;

/* Function used by Verrou to save the */
/* current rng state and replace it by the new seed */
void mcaquad_push_seed(uint64_t seed) {
  __rng_state = rng_state;
  _init_rng_state_struct(&rng_state, true, seed, false);
}

/* Function used by Verrou to restore the copied rng state */
void mcaquad_pop_seed() { rng_state = __rng_state; }

static const char *_get_error_mode_str(mcaquad_context_t *ctx) {
  if (ctx->relErr && ctx->absErr) {
    return MCAQUAD_ERR_MODE_STR[mcaquad_err_mode_all];
  } else if (ctx->relErr && !ctx->absErr) {
    return MCAQUAD_ERR_MODE_STR[mcaquad_err_mode_rel];
  } else if (!ctx->relErr && ctx->absErr) {
    return MCAQUAD_ERR_MODE_STR[mcaquad_err_mode_abs];
  } else {
    return NULL;
  }
}

/* noise = rand * 2^(exp) */
/* We can skip special cases since we never meet them */
/* Since we have exponent of float values, the result */
/* is comprised between: */
/* 127+127 = 254 < DOUBLE_EXP_MAX (1023)  */
/* -126-24+-126-24 = -300 > DOUBLE_EXP_MIN (-1022) */
double _noise_binary64(const int exp, rng_state_t *rng_state);
inline double _noise_binary64(const int exp, rng_state_t *rng_state) {
  const double d_rand = get_rand_double01(rng_state, &mcaquad_global_tid) - 0.5;
  binary64 b64 = {.f64 = d_rand};
  b64.ieee.exponent = b64.ieee.exponent + exp;
  return b64.f64;
}

/* noise = rand * 2^(exp) */
/* We can skip special cases since we never met them */
/* Since we have exponent of double values, the result */
/* is comprised between: */
/* 1023+1023 = 2046 < QUAD_EXP_MAX (16383)  */
/* -1022-53+-1022-53 = -2200 > QUAD_EXP_MIN (-16382) */
_Float128 _noise_binary128(const int exp, rng_state_t *rng_state) {
  /* random number in (-0.5, 0.5) */
  const _Float128 noise =
      (_Float128)get_rand_double01(rng_state, &mcaquad_global_tid) - 0.5Q;
  binary128 b128 = {.f128 = noise};
  b128.ieee128.exponent = b128.ieee128.exponent + exp;
  return b128.f128;
}

#define _IS_IEEE_MODE(CTX)                                                     \
  /* if mode ieee, do not introduce noise */                                   \
  (CTX->mode == mcaquad_mode_ieee)

#define _IS_NOT_NORMAL_OR_SUBNORMAL(X)                                         \
  /* Check that we are not in a special case */                                \
  (FPCLASSIFY(X) != FP_NORMAL && FPCLASSIFY(X) != FP_SUBNORMAL)

/* Macro function for checking if the value X must be noised */
#define _MUST_NOT_BE_NOISED(X, VIRTUAL_PRECISION, CTX)                         \
  /* if mode ieee, do not introduce noise */                                   \
  (CTX->mode == mcaquad_mode_ieee) ||					                                   \
  /* Check that we are not in a special case */				                         \
  (FPCLASSIFY(X) != FP_NORMAL && FPCLASSIFY(X) != FP_SUBNORMAL) ||	           \
  /* In RR if the number is representable in current virtual precision, */     \
  /* do not add any noise if */						                                     \
  (CTX->mode == mcaquad_mode_rr && _IS_REPRESENTABLE(X, VIRTUAL_PRECISION))

/* Generic function for computing the mca noise */
#define _NOISE(X, EXP, RNG_STATE)                                              \
  _Generic(X, double                                                           \
           : _noise_binary64, _Float128                                        \
           : _noise_binary128)(EXP, RNG_STATE)

/* Fast version of _INEXACT macro that adds noise in relative error.
  Always adds noise even if X is exact or sparsity is enabled.
 */
#define _FAST_INEXACT(X, VIRTUAL_PRECISION, CTX, RNG_STATE)                    \
  {                                                                            \
    mcaquad_context_t *TMP_CTX = (mcaquad_context_t *)CTX;                     \
    if (_IS_IEEE_MODE(TMP_CTX) || _IS_NOT_NORMAL_OR_SUBNORMAL(*X)) {           \
      return;                                                                  \
    }                                                                          \
    _init_rng_state_struct(&RNG_STATE, TMP_CTX->choose_seed,                   \
                           (unsigned long long)(TMP_CTX->seed), false);        \
    const int32_t e_a = GET_EXP_FLT(*X);                                       \
    const int32_t e_n_rel = e_a - (VIRTUAL_PRECISION - 1);                     \
    const typeof(*X) noise_rel = _NOISE(*X, e_n_rel, &RNG_STATE);              \
    *X = *X + noise_rel;                                                       \
  }

/* Macro function that adds mca noise to X
   according to the virtual_precision VIRTUAL_PRECISION */
#define _INEXACT(X, VIRTUAL_PRECISION, CTX, RNG_STATE)                         \
  {                                                                            \
    mcaquad_context_t *TMP_CTX = (mcaquad_context_t *)CTX;                     \
    _init_rng_state_struct(&RNG_STATE, TMP_CTX->choose_seed,                   \
                           (unsigned long long)(TMP_CTX->seed), false);        \
    if (_MUST_NOT_BE_NOISED(*X, VIRTUAL_PRECISION, TMP_CTX)) {                 \
      return;                                                                  \
    } else if (_mca_skip_eval(TMP_CTX->sparsity, &(RNG_STATE),                 \
                              &mcaquad_global_tid)) {                          \
      return;                                                                  \
    } else {                                                                   \
      if (TMP_CTX->relErr) {                                                   \
        const int32_t e_a = GET_EXP_FLT(*X);                                   \
        const int32_t e_n_rel = e_a - (VIRTUAL_PRECISION - 1);                 \
        const typeof(*X) noise_rel = _NOISE(*X, e_n_rel, &(RNG_STATE));        \
        *X += noise_rel;                                                       \
      }                                                                        \
      if (TMP_CTX->absErr) {                                                   \
        const int32_t e_n_abs = TMP_CTX->absErr_exp;                           \
        const typeof(*X) noise_abs = _NOISE(*X, e_n_abs, &(RNG_STATE));        \
        *X += noise_abs;                                                       \
      }                                                                        \
    }                                                                          \
  }

/* Adds the mca noise to da */
extern void _mcaquad_inexact_binary64(double *da, void *context);
inline void _mcaquad_inexact_binary64(double *da, void *context) {
  mcaquad_context_t *ctx = (mcaquad_context_t *)context;
  _INEXACT(da, ctx->binary32_precision, ctx, rng_state);
}

/* Adds the mca noise to qa */
extern void _mcaquad_inexact_binary128(_Float128 *qa, void *context);
inline void _mcaquad_inexact_binary128(_Float128 *qa, void *context) {
  mcaquad_context_t *ctx = (mcaquad_context_t *)context;
  _INEXACT(qa, ctx->binary64_precision, ctx, rng_state);
}

/* Generic functions that adds noise to A */
/* The function is choosen depending on the type of X  */
#define _INEXACT_BINARYN(X, A, CTX)                                            \
  _Generic(X, double                                                           \
           : _mcaquad_inexact_binary64, _Float128                              \
           : _mcaquad_inexact_binary128)(A, CTX)

/******************** MCA ARITHMETIC FUNCTIONS ********************
 * The following set of functions perform the MCA operation. Operands
 * are first converted to quad  format (GCC), inbound and outbound
 * perturbations are applied using the _mca_inexact function, and the
 * result converted to the original format for return
 *******************************************************************/

#define PERFORM_FMA(A, B, C)                                                   \
  _Generic(A, float                                                            \
           : interflop_fma_binary32, double                                    \
           : interflop_fma_binary64, _Float128                                 \
           : interflop_fma_binary128)(A, B, C)

/* perform_ternary_op: applies the ternary operator (op) to (a), (b)
 * and (c) */
/* and stores the result in (res) */
#define PERFORM_UNARY_OP(op, res, a)                                           \
  switch (op) {                                                                \
  case mcaquad_cast:                                                           \
    res = (float)(a);                                                          \
    break;                                                                     \
  default:                                                                     \
    logger_error("invalid operator %c", op);                                   \
  };

/* perform_bin_op: applies the binary operator (op) to (a) and (b) */
/* and stores the result in (res) */
#define PERFORM_BIN_OP(OP, RES, A, B)                                          \
  switch (OP) {                                                                \
  case mcaquad_add:                                                            \
    RES = (A) + (B);                                                           \
    break;                                                                     \
  case mcaquad_mul:                                                            \
    RES = (A) * (B);                                                           \
    break;                                                                     \
  case mcaquad_sub:                                                            \
    RES = (A) - (B);                                                           \
    break;                                                                     \
  case mcaquad_div:                                                            \
    RES = (A) / (B);                                                           \
    break;                                                                     \
  default:                                                                     \
    logger_error("invalid operator %c", OP);                                   \
  };

/* perform_ternary_op: applies the ternary operator (op) to (a), (b)
 * and (c) */
/* and stores the result in (res) */
#define PERFORM_TERNARY_OP(op, res, a, b, c)                                   \
  switch (op) {                                                                \
  case mcaquad_fma:                                                            \
    res = PERFORM_FMA(a, b, c);                                                \
    break;                                                                     \
  default:                                                                     \
    logger_error("invalid operator %c", op);                                   \
  };

/* Generic macro function that returns mca(A OP B) */
/* Functions are determined according to the type of X */
#define _MCAQUAD_UNARY_OP(A, OP, CTX, X)                                       \
  do {                                                                         \
    typeof(X) _A = A;                                                          \
    typeof(X) _RES = 0;                                                        \
    mcaquad_context_t *TMP_CTX = (mcaquad_context_t *)CTX;                     \
    if (TMP_CTX->daz) {                                                        \
      _A = DAZ(A);                                                             \
    }                                                                          \
    if (TMP_CTX->mode == mcaquad_mode_pb ||                                    \
        TMP_CTX->mode == mcaquad_mode_mca) {                                   \
      _INEXACT_BINARYN(X, &_A, CTX);                                           \
    }                                                                          \
    PERFORM_UNARY_OP(OP, _RES, _A);                                            \
    if (TMP_CTX->mode == mcaquad_mode_rr ||                                    \
        TMP_CTX->mode == mcaquad_mode_mca) {                                   \
      _INEXACT_BINARYN(X, &_RES, CTX);                                         \
    }                                                                          \
    if (TMP_CTX->ftz) {                                                        \
      _RES = FTZ((typeof(A))_RES);                                             \
    }                                                                          \
    return (typeof(A))(_RES);                                                  \
  } while (0);

/* Generic macro function that returns mca(A OP B) */
/* Functions are determined according to the type of X */
#define _MCAQUAD_BINARY_OP(A, B, OP, CTX, X)                                   \
  do {                                                                         \
    typeof(X) _A = A;                                                          \
    typeof(X) _B = B;                                                          \
    typeof(X) _RES = 0;                                                        \
    mcaquad_context_t *TMP_CTX = (mcaquad_context_t *)CTX;                     \
    if (TMP_CTX->daz) {                                                        \
      _A = DAZ(A);                                                             \
      _B = DAZ(B);                                                             \
    }                                                                          \
    if (TMP_CTX->mode == mcaquad_mode_pb ||                                    \
        TMP_CTX->mode == mcaquad_mode_mca) {                                   \
      _INEXACT_BINARYN(X, &_A, CTX);                                           \
      _INEXACT_BINARYN(X, &_B, CTX);                                           \
    }                                                                          \
    PERFORM_BIN_OP(OP, _RES, _A, _B);                                          \
    if (TMP_CTX->mode == mcaquad_mode_rr ||                                    \
        TMP_CTX->mode == mcaquad_mode_mca) {                                   \
      _INEXACT_BINARYN(X, &_RES, CTX);                                         \
    }                                                                          \
    if (TMP_CTX->ftz) {                                                        \
      _RES = FTZ((typeof(A))_RES);                                             \
    }                                                                          \
    return (typeof(A))(_RES);                                                  \
  } while (0);

/* Generic macro function that returns mca(A OP B OP C) */
/* Functions are determined according to the type of X */
#define _MCAQUAD_TERNARY_OP(A, B, C, OP, CTX, X)                               \
  do {                                                                         \
    typeof(X) _A = A;                                                          \
    typeof(X) _B = B;                                                          \
    typeof(X) _C = C;                                                          \
    typeof(X) _RES = 0;                                                        \
    mcaquad_context_t *TMP_CTX = (mcaquad_context_t *)CTX;                     \
    if (TMP_CTX->daz) {                                                        \
      _A = DAZ(A);                                                             \
      _B = DAZ(B);                                                             \
      _C = DAZ(C);                                                             \
    }                                                                          \
    if (TMP_CTX->mode == mcaquad_mode_pb ||                                    \
        TMP_CTX->mode == mcaquad_mode_mca) {                                   \
      _INEXACT_BINARYN(X, &_A, CTX);                                           \
      _INEXACT_BINARYN(X, &_B, CTX);                                           \
      _INEXACT_BINARYN(X, &_C, CTX);                                           \
    }                                                                          \
    PERFORM_TERNARY_OP(OP, _RES, _A, _B, _C);                                  \
    if (TMP_CTX->mode == mcaquad_mode_rr ||                                    \
        TMP_CTX->mode == mcaquad_mode_mca) {                                   \
      _INEXACT_BINARYN(X, &_RES, CTX);                                         \
    }                                                                          \
    if (TMP_CTX->ftz) {                                                        \
      _RES = FTZ((typeof(A))_RES);                                             \
    }                                                                          \
    return (typeof(A))(_RES);                                                  \
  } while (0);

/* Performs mca(dop a) where a is a binary32 value */
/* Intermediate computations are performed with binary64 */
inline float _mcaquad_binary32_unary_op(const float a, const mca_operations dop,
                                        void *context) {
  _MCAQUAD_UNARY_OP(a, dop, context, (double)0);
}

/* Performs mca(a dop b) where a and b are binary32 values */
/* Intermediate computations are performed with binary64 */
float _mcaquad_binary32_binary_op(const float a, const float b,
                                  const mca_operations dop, void *context);
inline float _mcaquad_binary32_binary_op(const float a, const float b,
                                         const mca_operations dop,
                                         void *context) {
  _MCAQUAD_BINARY_OP(a, b, dop, context, (double)0);
}

/* Performs mca(a dop b dop c) where a, b and c are binary32 values
 */
/* Intermediate computations are performed with binary64 */
float _mcaquad_binary32_ternary_op(const float a, const float b, const float c,
                                   const mca_operations dop, void *context);
inline float _mcaquad_binary32_ternary_op(const float a, const float b,
                                          const float c,
                                          const mca_operations dop,
                                          void *context) {
  _MCAQUAD_TERNARY_OP(a, b, c, dop, context, (double)0);
}

/* Performs mca(qop a) where a is a binary64 value */
/* Intermediate computations are performed with binary128 */
double _mcaquad_binary64_unary_op(const double a, const mca_operations qop,
                                  void *context);
inline double _mcaquad_binary64_unary_op(const double a,
                                         const mca_operations qop,
                                         void *context) {
  _MCAQUAD_UNARY_OP(a, qop, context, (_Float128)0);
}

/* Performs mca(a qop b) where a and b are binary64 values */
/* Intermediate computations are performed with binary128 */
double _mcaquad_binary64_binary_op(const double a, const double b,
                                   const mca_operations qop, void *context);
inline double _mcaquad_binary64_binary_op(const double a, const double b,
                                          const mca_operations qop,
                                          void *context) {
  _MCAQUAD_BINARY_OP(a, b, qop, context, (_Float128)0);
}

/* Performs mca(a qop b qop c) where a, b and c are binary64 values
 */
/* Intermediate computations are performed with binary128 */
double _mcaquad_binary64_ternary_op(const double a, const double b,
                                    const double c, const mca_operations qop,
                                    void *context);

inline double _mcaquad_binary64_ternary_op(const double a, const double b,
                                           const double c,
                                           const mca_operations qop,
                                           void *context) {
  _MCAQUAD_TERNARY_OP(a, b, c, qop, context, (_Float128)0);
}

/************************* FPHOOKS FUNCTIONS
 ************************** These functions correspond to those
 *inserted into the source code during source to source compilation
 *and are replacement to floating point operators
 **********************************************************************/

void INTERFLOP_MCAQUAD_API(add_float)(float a, float b, float *res,
                                      void *context) {
  *res = _mcaquad_binary32_binary_op(a, b, mcaquad_add, context);
}

void INTERFLOP_MCAQUAD_API(sub_float)(float a, float b, float *res,
                                      void *context) {
  *res = _mcaquad_binary32_binary_op(a, b, mcaquad_sub, context);
}

void INTERFLOP_MCAQUAD_API(mul_float)(float a, float b, float *res,
                                      void *context) {
  *res = _mcaquad_binary32_binary_op(a, b, mcaquad_mul, context);
}

void INTERFLOP_MCAQUAD_API(div_float)(float a, float b, float *res,
                                      void *context) {
  *res = _mcaquad_binary32_binary_op(a, b, mcaquad_div, context);
}

void INTERFLOP_MCAQUAD_API(fma_float)(float a, float b, float c, float *res,
                                      void *context) {
  *res = _mcaquad_binary32_ternary_op(a, b, c, mcaquad_fma, context);
}

void INTERFLOP_MCAQUAD_API(add_double)(double a, double b, double *res,
                                       void *context) {
  *res = _mcaquad_binary64_binary_op(a, b, mcaquad_add, context);
}

void INTERFLOP_MCAQUAD_API(sub_double)(double a, double b, double *res,
                                       void *context) {
  *res = _mcaquad_binary64_binary_op(a, b, mcaquad_sub, context);
}

void INTERFLOP_MCAQUAD_API(mul_double)(double a, double b, double *res,
                                       void *context) {
  *res = _mcaquad_binary64_binary_op(a, b, mcaquad_mul, context);
}

void INTERFLOP_MCAQUAD_API(div_double)(double a, double b, double *res,
                                       void *context) {
  *res = _mcaquad_binary64_binary_op(a, b, mcaquad_div, context);
}

void INTERFLOP_MCAQUAD_API(fma_double)(double a, double b, double c,
                                       double *res, void *context) {
  *res = _mcaquad_binary64_ternary_op(a, b, c, mcaquad_fma, context);
}

void INTERFLOP_MCAQUAD_API(cast_double_to_float)(double a, float *res,
                                                 void *context) {
  *res = _mcaquad_binary64_unary_op(a, mcaquad_cast, context);
}

void _interflop_usercall_inexact(void *context, va_list ap) {
  mcaquad_context_t *ctx = (mcaquad_context_t *)context;
  double xd = 0;
  _Float128 xq = 0;
  enum FTYPES ftype;
  void *value = NULL;
  int precision = 0, t = 0;
  ftype = va_arg(ap, enum FTYPES);
  value = va_arg(ap, void *);
  precision = va_arg(ap, int);
  switch (ftype) {
  case FFLOAT:
    xd = *((float *)value);
    t = (precision <= 0) ? (ctx->binary32_precision + precision) : precision;
    _FAST_INEXACT(&xd, t, context, rng_state);
    *((float *)value) = xd;
    break;
  case FDOUBLE:
    xq = *((double *)value);
    t = (precision <= 0) ? (ctx->binary64_precision + precision) : precision;
    _FAST_INEXACT(&xq, t, context, rng_state);
    *((double *)value) = xq;
    break;
  case FQUAD:
    xq = *((_Float128 *)value);
    _FAST_INEXACT(&xq, precision, context, rng_state);
    *((_Float128 *)value) = xq;
    break;
  default:
    logger_warning("Uknown type passed to "
                   "_interflop_usercall_inexact function");
    break;
  }
}

void INTERFLOP_MCAQUAD_API(user_call)(void *context, interflop_call_id id,
                                      va_list ap) {
  switch (id) {
  case INTERFLOP_INEXACT_ID:
    _interflop_usercall_inexact(context, ap);
    break;
  case INTERFLOP_SET_PRECISION_BINARY32:
    _set_mcaquad_precision_binary32(va_arg(ap, int), context);
    break;
  case INTERFLOP_SET_PRECISION_BINARY64:
    _set_mcaquad_precision_binary64(va_arg(ap, int), context);
    break;
  default:
    logger_warning("Unknown interflop_call id (=%d)", id);
    break;
  }
}

void _mcaquad_check_stdlib(void) {
  INTERFLOP_CHECK_IMPL(exit);
  INTERFLOP_CHECK_IMPL(fopen);
  INTERFLOP_CHECK_IMPL(fprintf);
  INTERFLOP_CHECK_IMPL(getenv);
  INTERFLOP_CHECK_IMPL(gettid);
  INTERFLOP_CHECK_IMPL(malloc);
  INTERFLOP_CHECK_IMPL(sprintf);
  INTERFLOP_CHECK_IMPL(strcasecmp);
  INTERFLOP_CHECK_IMPL(strerror);
  INTERFLOP_CHECK_IMPL(strtod);
  INTERFLOP_CHECK_IMPL(strtol);
  INTERFLOP_CHECK_IMPL(vfprintf);
  INTERFLOP_CHECK_IMPL(vwarnx);
  /* vfc_rng */
  INTERFLOP_CHECK_IMPL(gettimeofday);
}

void _mcaquad_alloc_context(void **context) {
  *context = (mcaquad_context_t *)interflop_malloc(sizeof(mcaquad_context_t));
}

static void _mcaquad_init_context(mcaquad_context_t *ctx) {
  ctx->mode = MCAQUAD_MODE_DEFAULT;
  ctx->binary32_precision = MCAQUAD_PRECISION_BINARY32_DEFAULT;
  ctx->binary64_precision = MCAQUAD_PRECISION_BINARY64_DEFAULT;
  ctx->relErr = true;
  ctx->absErr = false;
  ctx->absErr_exp = MCAQUAD_ABSOLUTE_ERROR_EXPONENT_DEFAULT;
  ctx->choose_seed = false;
  ctx->daz = MCAQUAD_DAZ_DEFAULT;
  ctx->ftz = MCAQUAD_FTZ_DEFAULT;
  ctx->seed = MCAQUAD_SEED_DEFAULT;
  ctx->sparsity = MCAQUAD_SPARSITY_DEFAULT;
}

void INTERFLOP_MCAQUAD_API(pre_init)(interflop_panic_t panic, File *stream,
                                     void **context) {
  interflop_set_handler("panic", panic);
  _mcaquad_check_stdlib();

  /* Initialize the logger */
  logger_init(panic, stream, backend_name);

  /* Allocate the context */
  _mcaquad_alloc_context(context);
  _mcaquad_init_context((mcaquad_context_t *)*context);
}

static struct argp_option options[] = {
    {key_prec_b32_str, KEY_PREC_B32, "PRECISION", 0,
     "select precision for binary32 (PRECISION > 0)", 0},
    {key_prec_b64_str, KEY_PREC_B64, "PRECISION", 0,
     "select precision for binary64 (PRECISION > 0)", 0},
    {key_mode_str, KEY_MODE, "MODE", 0,
     "select MCA mode among {ieee, mca, pb, rr}", 0},
    {key_err_mode_str, KEY_ERR_MODE, "ERROR_MODE", 0,
     "select error mode among {rel, abs, all}", 0},
    {key_err_exp_str, KEY_ERR_EXP, "MAX_ABS_ERROR_EXPONENT", 0,
     "select magnitude of the maximum absolute error", 0},
    {key_seed_str, KEY_SEED, "SEED", 0, "fix the random generator seed", 0},
    {key_daz_str, KEY_DAZ, 0, 0,
     "denormals-are-zero: sets denormals inputs to zero", 0},
    {key_ftz_str, KEY_FTZ, 0, 0, "flush-to-zero: sets denormal output to zero",
     0},
    {key_sparsity_str, KEY_SPARSITY, "SPARSITY", 0,
     "one in {sparsity} operations will be perturbed. 0 < sparsity "
     "<= 1.",
     0},
    {0}};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  mcaquad_context_t *ctx = (mcaquad_context_t *)state->input;
  char *endptr;
  int val = -1;
  int error = 0;
  float sparsity = MCAQUAD_SPARSITY_DEFAULT;
  uint64_t seed = MCAQUAD_SEED_DEFAULT;
  long exponent_error = MCAQUAD_ABSOLUTE_ERROR_EXPONENT_DEFAULT;
  switch (key) {
  case KEY_PREC_B32:
    /* precision for binary32 */
    error = 0;
    val = interflop_strtol(arg, &endptr, &error);
    if (error != 0 || val <= 0) {
      logger_error("--%s invalid value provided, must be a positive integer",
                   key_prec_b32_str);
    } else {
      _set_mcaquad_precision_binary32(val, ctx);
    }
    break;
  case KEY_PREC_B64:
    /* precision for binary64 */
    error = 0;
    val = interflop_strtol(arg, &endptr, &error);
    if (error != 0 || val <= 0) {
      logger_error("--%s invalid value provided, must be a positive integer",
                   key_prec_b64_str);
    } else {
      _set_mcaquad_precision_binary64(val, ctx);
    }
    break;
  case KEY_MODE:
    /* mca mode */
    if (interflop_strcasecmp(MCAQUAD_MODE_STR[mcaquad_mode_ieee], arg) == 0) {
      _set_mcaquad_mode(mcaquad_mode_ieee, ctx);
    } else if (interflop_strcasecmp(MCAQUAD_MODE_STR[mcaquad_mode_mca], arg) ==
               0) {
      _set_mcaquad_mode(mcaquad_mode_mca, ctx);
    } else if (interflop_strcasecmp(MCAQUAD_MODE_STR[mcaquad_mode_pb], arg) ==
               0) {
      _set_mcaquad_mode(mcaquad_mode_pb, ctx);
    } else if (interflop_strcasecmp(MCAQUAD_MODE_STR[mcaquad_mode_rr], arg) ==
               0) {
      _set_mcaquad_mode(mcaquad_mode_rr, ctx);
    } else {
      logger_error("--%s invalid value provided, must be one of: "
                   "{ieee, mca, pb, rr}.",
                   key_mode_str);
    }
    break;
  case KEY_ERR_MODE:
    /* mca error mode */
    if (interflop_strcasecmp(MCAQUAD_ERR_MODE_STR[mcaquad_err_mode_rel], arg) ==
        0) {
      _set_mcaquad_error_mode(mcaquad_err_mode_rel, ctx);
    } else if (interflop_strcasecmp(MCAQUAD_ERR_MODE_STR[mcaquad_err_mode_abs],
                                    arg) == 0) {
      _set_mcaquad_error_mode(mcaquad_err_mode_abs, ctx);
    } else if (interflop_strcasecmp(MCAQUAD_ERR_MODE_STR[mcaquad_err_mode_all],
                                    arg) == 0) {
      _set_mcaquad_error_mode(mcaquad_err_mode_all, ctx);
    } else {
      logger_error("--%s invalid value provided, must be one of: "
                   "{rel, abs, all}.",
                   key_err_mode_str);
    }
    break;
  case KEY_ERR_EXP:
    /* exponent of the maximum absolute error */
    error = 0;
    exponent_error = interflop_strtol(arg, &endptr, &error);
    if (error != 0) {
      logger_error("--%s invalid value provided, must be an integer",
                   key_err_exp_str);
    }
    _set_mcaquad_max_abs_err_exp(exponent_error, ctx);
    break;
  case KEY_SEED:
    /* seed */
    error = 0;
    seed = interflop_strtol(arg, &endptr, &error);
    if (error != 0) {
      logger_error("--%s invalid value provided, must be an integer",
                   key_seed_str);
    }
    _set_mcaquad_seed(seed, ctx);
    break;
  case KEY_DAZ:
    /* denormals-are-zero */
    _set_mcaquad_daz(true, ctx);
    break;
  case KEY_FTZ:
    /* flush-to-zero */
    _set_mcaquad_ftz(true, ctx);
    break;
  case KEY_SPARSITY:
    /* sparse perturbations */
    error = 0;
    sparsity = interflop_strtod(arg, &endptr, &error);
    if (sparsity <= 0) {
      error = 1;
    }
    if (error != 0) {
      logger_error("--%s invalid value provided, must be positive",
                   key_sparsity_str);
    }
    _set_mcaquad_sparsity(sparsity, ctx);
    break;
  default:
    return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

static struct argp argp = {options, parse_opt, "", "", NULL, NULL, NULL};

void INTERFLOP_MCAQUAD_API(cli)(int argc, char **argv, void *context) {
  /* parse backend arguments */
  mcaquad_context_t *ctx = (mcaquad_context_t *)context;
  if (interflop_argp_parse != NULL) {
    interflop_argp_parse(&argp, argc, argv, 0, 0, ctx);
  } else {
    interflop_panic("Interflop backend error: argp_parse not implemented\n"
                    "Provide implementation or use interflop_configure to "
                    "configure the backend\n");
  }
}

void INTERFLOP_MCAQUAD_API(configure)(void *configure, void *context) {
  mcaquad_context_t *ctx = (mcaquad_context_t *)context;
  mcaquad_conf_t *conf = (mcaquad_conf_t *)configure;
  _set_mcaquad_seed(conf->seed, ctx);
  _set_mcaquad_sparsity(conf->sparsity, ctx);
  _set_mcaquad_precision_binary32(conf->precision_binary32, ctx);
  _set_mcaquad_precision_binary64(conf->precision_binary64, ctx);
  _set_mcaquad_mode(conf->mode, ctx);
  _set_mcaquad_error_mode(conf->err_mode, ctx);
  if (conf->max_abs_err_exponent != (unsigned int)(-1)) {
    _set_mcaquad_max_abs_err_exp(conf->max_abs_err_exponent, ctx);
  }
  _set_mcaquad_daz(conf->daz, ctx);
  _set_mcaquad_ftz(conf->ftz, ctx);
}

static void print_information_header(void *context) {
  /* Environnement variable to disable loading message */
  char *silent_load_env = interflop_getenv("VFC_BACKENDS_SILENT_LOAD");
  bool silent_load = ((silent_load_env == NULL) ||
                      (interflop_strcasecmp(silent_load_env, "True") != 0))
                         ? false
                         : true;

  if (silent_load)
    return;

  mcaquad_context_t *ctx = (mcaquad_context_t *)context;
  logger_info("load backend with:\n");
  logger_info("%s = %d\n", key_prec_b32_str, ctx->binary32_precision);
  logger_info("%s = %d\n", key_prec_b64_str, ctx->binary64_precision);
  logger_info("%s = %s\n", key_mode_str, MCAQUAD_MODE_STR[ctx->mode]);
  logger_info("%s = %s\n", key_err_mode_str, _get_error_mode_str(ctx));
  logger_info("%s = %d\n", key_err_exp_str, ctx->absErr_exp);
  logger_info("%s = %s\n", key_daz_str, ctx->daz ? "true" : "false");
  logger_info("%s = %s\n", key_ftz_str, ctx->ftz ? "true" : "false");
  logger_info("%s = %f\n", key_sparsity_str, ctx->sparsity);
  logger_info("%s = %lu\n", key_seed_str, ctx->seed);
}

struct interflop_backend_interface_t
INTERFLOP_MCAQUAD_API(init)(void *context) {

  mcaquad_context_t *ctx = (mcaquad_context_t *)context;

  print_information_header(ctx);

  struct interflop_backend_interface_t interflop_backend_mcaquad = {
    interflop_add_float : INTERFLOP_MCAQUAD_API(add_float),
    interflop_sub_float : INTERFLOP_MCAQUAD_API(sub_float),
    interflop_mul_float : INTERFLOP_MCAQUAD_API(mul_float),
    interflop_div_float : INTERFLOP_MCAQUAD_API(div_float),
    interflop_cmp_float : NULL,
    interflop_add_double : INTERFLOP_MCAQUAD_API(add_double),
    interflop_sub_double : INTERFLOP_MCAQUAD_API(sub_double),
    interflop_mul_double : INTERFLOP_MCAQUAD_API(mul_double),
    interflop_div_double : INTERFLOP_MCAQUAD_API(div_double),
    interflop_cmp_double : NULL,
    interflop_cast_double_to_float :
        INTERFLOP_MCAQUAD_API(cast_double_to_float),
    interflop_fma_float : INTERFLOP_MCAQUAD_API(fma_float),
    interflop_fma_double : INTERFLOP_MCAQUAD_API(fma_double),
    interflop_enter_function : NULL,
    interflop_exit_function : NULL,
    interflop_user_call : INTERFLOP_MCAQUAD_API(user_call),
    interflop_finalize : NULL,
  };

  /* The seed for the RNG is initialized upon the first request for a
     random number */
  _init_rng_state_struct(&rng_state, ctx->choose_seed, ctx->seed, false);

  return interflop_backend_mcaquad;
}

struct interflop_backend_interface_t interflop_init(void *context)
    __attribute__((weak, alias("interflop_mcaquad_init")));

void interflop_pre_init(interflop_panic_t panic, File *stream, void **context)
    __attribute__((weak, alias("interflop_mcaquad_pre_init")));

void interflop_cli(int argc, char **argv, void *context)
    __attribute__((weak, alias("interflop_mcaquad_cli")));
