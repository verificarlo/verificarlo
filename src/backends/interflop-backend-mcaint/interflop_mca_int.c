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
 *  Copyright (c) 2019-2024                                                  *\
 *     Verificarlo Contributors                                              *\
 *                                                                           *\
 ****************************************************************************/

#include <argp.h>
#include <err.h>
#include <math.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <threads.h>
#include <unistd.h>

#include "interflop/common/float_const.h"
#include "interflop/common/float_struct.h"
#include "interflop/common/float_utils.h"
#include "interflop/common/options.h"
#include "interflop/fma/interflop_fma.h"
#include "interflop/interflop.h"
#include "interflop/interflop_stdlib.h"
#include "interflop/iostream/logger.h"
#include "interflop/rng/vfc_rng.h"
#include "interflop_mca_int.h"

/* Disable thread safety for RNG required for Valgrind */
#ifdef RNG_THREAD_SAFE
#define TLS __thread
#else
#define TLS
#endif

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

static const char backend_name[] = "interflop-mcaint";
static const char backend_version[] = "1.x-dev";

static const char key_prec_b32_str[] = "precision-binary32";
static const char key_prec_b64_str[] = "precision-binary64";
static const char key_mode_str[] = "mode";
static const char key_err_mode_str[] = "error-mode";
static const char key_err_exp_str[] = "max-abs-error-exponent";
static const char key_seed_str[] = "seed";
static const char key_daz_str[] = "daz";
static const char key_ftz_str[] = "ftz";
static const char key_sparsity_str[] = "sparsity";

static const char *MCAINT_MODE_STR[] = {[mcaint_mode_ieee] = "ieee",
                                        [mcaint_mode_mca] = "mca",
                                        [mcaint_mode_pb] = "pb",
                                        [mcaint_mode_rr] = "rr"};

static const char *MCAINT_ERR_MODE_STR[] = {[mcaint_err_mode_rel] = "rel",
                                            [mcaint_err_mode_abs] = "abs",
                                            [mcaint_err_mode_all] = "all"};

/* possible operations values */
typedef enum {
  mcaint_add = '+',
  mcaint_sub = '-',
  mcaint_mul = '*',
  mcaint_div = '/',
  mcaint_fma = 'f',
  mcaint_cast = 'c',
} mcaint_operations;

/******************** MCA CONTROL FUNCTIONS *******************
 * The following functions are used to set virtual precision and
 * MCA mode of operation.
 ***************************************************************/

/* Set the mca mode */
static void _set_mcaint_mode(const mcaint_mode mode, void *context) {
  mcaint_context_t *ctx = (mcaint_context_t *)context;

  if (mode >= _mcaint_mode_end_) {
    logger_error("--%s invalid value provided, must be one of: "
                 "{ieee, mca, pb, rr}.",
                 key_mode_str);
  }
  ctx->mode = mode;
}

/* Set the virtual precision for binary32 */
static void _set_mcaint_precision_binary32(const int precision, void *context) {
  mcaint_context_t *ctx = (mcaint_context_t *)context;
  _set_precision(MCAINT, precision, ctx->binary32_precision, (float)0);
}

/* Set the virtual precision for binary64 */
static void _set_mcaint_precision_binary64(const int precision, void *context) {
  mcaint_context_t *ctx = (mcaint_context_t *)context;
  _set_precision(MCAINT, precision, ctx->binary64_precision, (double)0);
}

/* Set the error mode */
static void _set_mcaint_error_mode(mcaint_err_mode mode,
                                   mcaint_context_t *ctx) {
  if (mode >= _mcaint_err_mode_end_) {
    logger_error("invalid error mode provided, must be one of: "
                 "{rel, abs, all}.");
  } else {
    switch (mode) {
    case mcaint_err_mode_rel:
      ctx->relErr = true;
      ctx->absErr = false;
      break;
    case mcaint_err_mode_abs:
      ctx->relErr = false;
      ctx->absErr = true;
      break;
    case mcaint_err_mode_all:
      ctx->relErr = true;
      ctx->absErr = true;
    default:
      break;
    }
  }
}

/* Get the error mode as string */
static const char *_get_mcaint_error_mode_str(mcaint_context_t *ctx) {
  if (ctx->relErr && ctx->absErr) {
    return MCAINT_ERR_MODE_STR[mcaint_err_mode_all];
  } else if (ctx->relErr && !ctx->absErr) {
    return MCAINT_ERR_MODE_STR[mcaint_err_mode_rel];
  } else if (!ctx->relErr && ctx->absErr) {
    return MCAINT_ERR_MODE_STR[mcaint_err_mode_abs];
  } else {
    return NULL;
  }
}

/* Set the maximal absolute error exponent */
static void _set_mcaint_max_abs_err_exp(long exponent, mcaint_context_t *ctx) {
  ctx->absErr_exp = exponent;
}

/* Set Denormals-Are-Zero flag */
static void _set_mcaint_daz(bool daz, mcaint_context_t *ctx) { ctx->daz = daz; }

/* Set Flush-To-Zero flag */
static void _set_mcaint_ftz(bool ftz, mcaint_context_t *ctx) { ctx->ftz = ftz; }

/* Set sparsity value */
static void _set_mcaint_sparsity(float sparsity, mcaint_context_t *ctx) {
  if (sparsity <= 0) {
    logger_error("invalid value for sparsity %d, must be positive");
  } else {
    ctx->sparsity = sparsity;
  }
}

/* Set RNG seed */
static void _set_mcaint_seed(uint64_t seed, mcaint_context_t *ctx) {
  ctx->choose_seed = true;
  ctx->seed = seed;
}

const char *get_mcaint_mode_name(mcaint_mode mode) {
  if (mode >= _mcaint_mode_end_) {
    return NULL;
  } else {
    return MCAINT_MODE_STR[mode];
  }
}

/******************** MCA RANDOM FUNCTIONS ********************
 * The following functions are used to calculate the random
 * perturbations used for MCA
 ***************************************************************/

/* global thread identifier */
pid_t mcaint_global_tid = 0;

/* helper data structure to centralize the data used for random number
 * generation */
static TLS rng_state_t rng_state;
/* copy */
static TLS rng_state_t __rng_state;

/* Function used by Verrou to save the */
/* current rng state and replace it by the new seed */
void mcaint_push_seed(uint64_t seed) {
  __rng_state = rng_state;
  _init_rng_state_struct(&rng_state, true, seed, false);
}

/* Function used by Verrou to restore the copied rng state */
void mcaint_pop_seed() { rng_state = __rng_state; }

/* noise = rand * 2^(exp) */
/* We can skip special cases since we never meet them */
/* Since we have exponent of float values, the result */
/* is comprised between: */
/* 127+127 = 254 < DOUBLE_EXP_MAX (1023)  */
/* -126-24+-126-24 = -300 > DOUBLE_EXP_MIN (-1022) */
static inline void _noise_binary64(double *x, const int exp,
                                   rng_state_t *rng_state) {
  // Convert preserving-bytes double to int64_t
  binary64 *b64 = (binary64 *)x;

  // amount by which to shift the noise term sign (1) + exp (11) + noise
  // exponent
  const uint32_t shift = 1 + DOUBLE_EXP_SIZE - exp;

  // noise is a signed integer so the noise is centered around 0
  int64_t noise = get_rand_uint64(rng_state, &mcaint_global_tid);

  // right shift the noise to the correct magnitude, this is a arithmetic
  // shift and sign bit will be extended
  noise >>= shift;

  // Add the noise to the x value
  b64->s64 += noise;
}

/* noise = rand * 2^(exp) */
/* We can skip special cases since we never meet them */
/* Since we have exponent of double values, the result */
/* is comprised between: */
/* 1023+1023 = 2046 < QUAD_EXP_MAX (16383)  */
/* -1022-53+-1022-53 = -2200 > QUAD_EXP_MIN (-16382) */
static void _noise_binary128(_Float128 *x, const int exp,
                             rng_state_t *rng_state) {

  // Convert preserving-bytes _Float128 to __int128
  binary128 *b128 = (binary128 *)x;

  // amount by which to shift the noise term sign (1) + exp (15) + noise
  // exponent
  const uint32_t shift = 1 + QUAD_EXP_SIZE - exp;

  // Generate 128 signed noise
  // only 64 bits of noise are used, they are left aligned in a signed 64 bit
  binary128 noise = {.words64.high =
                         get_rand_uint64(rng_state, &mcaint_global_tid)};

  // right shift the noise to the correct magnitude, this is a arithmetic
  // shift and sign bit will be extended
  noise.i128 >>= shift;

  // Add the noise
  b128->i128 += noise.i128;
}

/* Macro function for checking if the value X must be noised */
#define _MUST_NOT_BE_NOISED(X, VIRTUAL_PRECISION, CTX)                         \
  /* if mode ieee, do not introduce noise */                                   \
  (CTX->mode == mcaint_mode_ieee) || \
  /* Check that we are not in a special case */ \
  (FPCLASSIFY(X) != FP_NORMAL && FPCLASSIFY(X) != FP_SUBNORMAL) ||         \
  /* In RR if the number is representable in current virtual precision, */ \
  /* do not add any noise if */                                           \
  (CTX->mode == mcaint_mode_rr && _IS_REPRESENTABLE(X, VIRTUAL_PRECISION))

/* Generic function for computing the mca noise */
#define _NOISE(X, EXP, RNG_STATE)                                              \
  _Generic(*X, double: _noise_binary64, _Float128: _noise_binary128)(          \
      X, EXP, RNG_STATE)

/* Macro function that adds mca noise to X
   according to the virtual_precision VIRTUAL_PRECISION */
#define _INEXACT(X, VIRTUAL_PRECISION, CTX, RNG_STATE)                         \
  {                                                                            \
    mcaint_context_t *TMP_CTX = (mcaint_context_t *)CTX;                       \
    _init_rng_state_struct(&RNG_STATE, TMP_CTX->choose_seed,                   \
                           (unsigned long long)(TMP_CTX->seed), false);        \
    if (_MUST_NOT_BE_NOISED(*X, VIRTUAL_PRECISION, TMP_CTX)) {                 \
      return;                                                                  \
    } else if (_mca_skip_eval(TMP_CTX->sparsity, &(RNG_STATE),                 \
                              &mcaint_global_tid)) {                           \
      return;                                                                  \
    } else {                                                                   \
      const int32_t e_n_rel = -(VIRTUAL_PRECISION - 1);                        \
      _NOISE(X, e_n_rel, &RNG_STATE);                                          \
    }                                                                          \
  }

/* Adds the mca noise to da */
static void _mcaint_inexact_binary64(double *da, void *context) {
  mcaint_context_t *ctx = (mcaint_context_t *)context;
  _INEXACT(da, ctx->binary32_precision, ctx, rng_state);
}

/* Adds the mca noise to qa */
static void _mcaint_inexact_binary128(_Float128 *qa, void *context) {
  mcaint_context_t *ctx = (mcaint_context_t *)context;
  _INEXACT(qa, ctx->binary64_precision, ctx, rng_state);
}

/* Generic functions that adds noise to A */
/* The function is choosen depending on the type of X  */
#define _INEXACT_BINARYN(X, A, CTX)                                            \
  _Generic(X,                                                                  \
      double: _mcaint_inexact_binary64,                                        \
      _Float128: _mcaint_inexact_binary128)(A, CTX)

/******************** MCA ARITHMETIC FUNCTIONS ********************
 * The following set of functions perform the MCA operation. Operands
 * are first converted to quad  format (GCC), inbound and outbound
 * perturbations are applied using the _mcaint_inexact function, and the
 * result converted to the original format for return
 *******************************************************************/

#define PERFORM_FMA(A, B, C)                                                   \
  _Generic(A,                                                                  \
      float: interflop_fma_binary32,                                           \
      double: interflop_fma_binary64,                                          \
      _Float128: interflop_fma_binary128)(A, B, C)

/* perform_ternary_op: applies the ternary operator (op) to (a) */
/* and stores the result in (res) */
#define PERFORM_UNARY_OP(op, res, a)                                           \
  switch (op) {                                                                \
  case mcaint_cast:                                                            \
    res = (float)(a);                                                          \
    break;                                                                     \
  default:                                                                     \
    logger_error("invalid operator %c", op);                                   \
  };

/* perform_bin_op: applies the binary operator (op) to (a) and (b) */
/* and stores the result in (res) */
#define PERFORM_BIN_OP(OP, RES, A, B)                                          \
  switch (OP) {                                                                \
  case mcaint_add:                                                             \
    RES = (A) + (B);                                                           \
    break;                                                                     \
  case mcaint_mul:                                                             \
    RES = (A) * (B);                                                           \
    break;                                                                     \
  case mcaint_sub:                                                             \
    RES = (A) - (B);                                                           \
    break;                                                                     \
  case mcaint_div:                                                             \
    RES = (A) / (B);                                                           \
    break;                                                                     \
  default:                                                                     \
    logger_error("invalid operator %c", OP);                                   \
  };

/* perform_ternary_op: applies the ternary operator (op) to (a), (b) and (c) */
/* and stores the result in (res) */
#define PERFORM_TERNARY_OP(op, res, a, b, c)                                   \
  switch (op) {                                                                \
  case mcaint_fma:                                                             \
    res = PERFORM_FMA((a), (b), (c));                                          \
    break;                                                                     \
  default:                                                                     \
    logger_error("invalid operator %c", op);                                   \
  };

/* Generic macro function that returns mca(OP(A)) */
/* Functions are determined according to the type of X */
#define _MCAINT_UNARY_OP(A, OP, CTX, X)                                        \
  do {                                                                         \
    typeof(X) _A = A;                                                          \
    typeof(X) _RES = 0;                                                        \
    mcaint_context_t *TMP_CTX = (mcaint_context_t *)CTX;                       \
    if (TMP_CTX->daz) {                                                        \
      _A = DAZ(A);                                                             \
    }                                                                          \
    if (TMP_CTX->mode == mcaint_mode_pb || TMP_CTX->mode == mcaint_mode_mca) { \
      _INEXACT_BINARYN(X, &_A, CTX);                                           \
    }                                                                          \
    PERFORM_UNARY_OP(OP, _RES, _A);                                            \
    if (TMP_CTX->mode == mcaint_mode_rr || TMP_CTX->mode == mcaint_mode_mca) { \
      _INEXACT_BINARYN(X, &_RES, CTX);                                         \
    }                                                                          \
    if (TMP_CTX->ftz) {                                                        \
      _RES = FTZ((typeof(A))_RES);                                             \
    }                                                                          \
    return (typeof(A))(_RES);                                                  \
  } while (0);

/* Generic macro function that returns mca(OP(A,B)) */
/* Functions are determined according to the type of X */
#define _MCAINT_BINARY_OP(A, B, OP, CTX, X)                                    \
  do {                                                                         \
    typeof(X) _A = A;                                                          \
    typeof(X) _B = B;                                                          \
    typeof(X) _RES = 0;                                                        \
    mcaint_context_t *TMP_CTX = (mcaint_context_t *)CTX;                       \
    if (TMP_CTX->daz) {                                                        \
      _A = DAZ(A);                                                             \
      _B = DAZ(B);                                                             \
    }                                                                          \
    if (TMP_CTX->mode == mcaint_mode_pb || TMP_CTX->mode == mcaint_mode_mca) { \
      _INEXACT_BINARYN(X, &_A, CTX);                                           \
      _INEXACT_BINARYN(X, &_B, CTX);                                           \
    }                                                                          \
    PERFORM_BIN_OP(OP, _RES, _A, _B);                                          \
    if (TMP_CTX->mode == mcaint_mode_rr || TMP_CTX->mode == mcaint_mode_mca) { \
      _INEXACT_BINARYN(X, &_RES, CTX);                                         \
    }                                                                          \
    if (TMP_CTX->ftz) {                                                        \
      _RES = FTZ((typeof(A))_RES);                                             \
    }                                                                          \
    return (typeof(A))(_RES);                                                  \
  } while (0);

/* Generic macro function that returns mca(OP(A,B,C)) */
/* Functions are determined according to the type of X */
#define _MCAINT_TERNARY_OP(A, B, C, OP, CTX, X)                                \
  do {                                                                         \
    typeof(X) _A = A;                                                          \
    typeof(X) _B = B;                                                          \
    typeof(X) _C = C;                                                          \
    typeof(X) _RES = 0;                                                        \
    mcaint_context_t *TMP_CTX = (mcaint_context_t *)CTX;                       \
    if (TMP_CTX->daz) {                                                        \
      _A = DAZ(A);                                                             \
      _B = DAZ(B);                                                             \
      _C = DAZ(C);                                                             \
    }                                                                          \
    if (TMP_CTX->mode == mcaint_mode_pb || TMP_CTX->mode == mcaint_mode_mca) { \
      _INEXACT_BINARYN(X, &_A, CTX);                                           \
      _INEXACT_BINARYN(X, &_B, CTX);                                           \
      _INEXACT_BINARYN(X, &_C, CTX);                                           \
    }                                                                          \
    PERFORM_TERNARY_OP(OP, _RES, _A, _B, _C);                                  \
    if (TMP_CTX->mode == mcaint_mode_rr || TMP_CTX->mode == mcaint_mode_mca) { \
      _INEXACT_BINARYN(X, &_RES, CTX);                                         \
    }                                                                          \
    if (TMP_CTX->ftz) {                                                        \
      _RES = FTZ((typeof(A))_RES);                                             \
    }                                                                          \
    return (typeof(A))(_RES);                                                  \
  } while (0);

/* Performs mca(dop a) where a is a binary32 value */
/* Intermediate computations are performed with binary64 */
static inline float _mcaint_binary32_unary_op(const float a,
                                              const mcaint_operations dop,
                                              void *context) {
  _MCAINT_UNARY_OP(a, dop, context, (double)0);
}

/* Performs mca(a dop b) where a and b are binary32 values */
/* Intermediate computations are performed with binary64 */
static inline float _mcaint_binary32_binary_op(const float a, const float b,
                                               const mcaint_operations dop,
                                               void *context) {
  _MCAINT_BINARY_OP(a, b, dop, context, (double)0);
}

/* Performs mca(a dop b dop c) where a, b and c are binary32 values */
/* Intermediate computations are performed with binary64 */
// float _mcaint_binary32_ternary_op(const float a, const float b, const float
// c,
//                                const mcaint_operations dop, void *context);

static inline float _mcaint_binary32_ternary_op(const float a, const float b,
                                                const float c,
                                                const mcaint_operations dop,
                                                void *context) {
  _MCAINT_TERNARY_OP(a, b, c, dop, context, (double)0);
}

/* Performs mca(qop a) where a is a binary64 value */
/* Intermediate computations are performed with binary128 */
static inline double _mcaint_binary64_unary_op(const double a,
                                               const mcaint_operations qop,
                                               void *context) {
  _MCAINT_UNARY_OP(a, qop, context, (_Float128)0);
}

/* Performs mca(a qop b) where a and b are binary64 values */
/* Intermediate computations are performed with binary128 */
static inline double _mcaint_binary64_binary_op(const double a, const double b,
                                                const mcaint_operations qop,
                                                void *context) {
  _MCAINT_BINARY_OP(a, b, qop, context, (_Float128)0);
}

/* Performs mca(a qop b qop c) where a, b and c are binary64 values */
/* Intermediate computations are performed with binary128 */
static inline double _mcaint_binary64_ternary_op(const double a, const double b,
                                                 const double c,
                                                 const mcaint_operations qop,
                                                 void *context) {
  _MCAINT_TERNARY_OP(a, b, c, qop, context, (_Float128)0);
}

/************************* FPHOOKS FUNCTIONS *************************
 * These functions correspond to those inserted into the source code
 * during source to source compilation and are replacement to floating
 * point operators
 **********************************************************************/

void INTERFLOP_MCAINT_API(add_float)(float a, float b, float *res,
                                     void *context) {
  *res = _mcaint_binary32_binary_op(a, b, mcaint_add, context);
}

void INTERFLOP_MCAINT_API(sub_float)(float a, float b, float *res,
                                     void *context) {
  *res = _mcaint_binary32_binary_op(a, b, mcaint_sub, context);
}

void INTERFLOP_MCAINT_API(mul_float)(float a, float b, float *res,
                                     void *context) {
  *res = _mcaint_binary32_binary_op(a, b, mcaint_mul, context);
}

void INTERFLOP_MCAINT_API(div_float)(float a, float b, float *res,
                                     void *context) {
  *res = _mcaint_binary32_binary_op(a, b, mcaint_div, context);
}

void INTERFLOP_MCAINT_API(fma_float)(float a, float b, float c, float *res,
                                     void *context) {
  *res = _mcaint_binary32_ternary_op(a, b, c, mcaint_fma, context);
}

void INTERFLOP_MCAINT_API(add_double)(double a, double b, double *res,
                                      void *context) {
  *res = _mcaint_binary64_binary_op(a, b, mcaint_add, context);
}

void INTERFLOP_MCAINT_API(sub_double)(double a, double b, double *res,
                                      void *context) {
  *res = _mcaint_binary64_binary_op(a, b, mcaint_sub, context);
}

void INTERFLOP_MCAINT_API(mul_double)(double a, double b, double *res,
                                      void *context) {
  *res = _mcaint_binary64_binary_op(a, b, mcaint_mul, context);
}

void INTERFLOP_MCAINT_API(div_double)(double a, double b, double *res,
                                      void *context) {
  *res = _mcaint_binary64_binary_op(a, b, mcaint_div, context);
}

void INTERFLOP_MCAINT_API(fma_double)(double a, double b, double c, double *res,
                                      void *context) {
  *res = _mcaint_binary64_ternary_op(a, b, c, mcaint_fma, context);
}

void INTERFLOP_MCAINT_API(cast_double_to_float)(double a, float *res,
                                                void *context) {
  *res = _mcaint_binary64_unary_op(a, mcaint_cast, context);
}

const char *INTERFLOP_MCAINT_API(get_backend_name)(void) {
  return backend_name;
}

const char *INTERFLOP_MCAINT_API(get_backend_version)(void) {
  return backend_version;
}

void _mcaint_check_stdlib(void) {
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

void _mcaint_alloc_context(void **context) {
  *context = (mcaint_context_t *)interflop_malloc(sizeof(mcaint_context_t));
}

static void _mcaint_init_context(mcaint_context_t *ctx) {
  ctx->mode = MCAINT_MODE_DEFAULT;
  ctx->binary32_precision = MCAINT_PRECISION_BINARY32_DEFAULT;
  ctx->binary64_precision = MCAINT_PRECISION_BINARY64_DEFAULT;
  ctx->relErr = true;
  ctx->absErr = false;
  ctx->absErr_exp = MCAINT_ABSOLUTE_ERROR_EXPONENT_DEFAULT;
  ctx->choose_seed = false;
  ctx->daz = MCAINT_DAZ_DEFAULT;
  ctx->ftz = MCAINT_FTZ_DEFAULT;
  ctx->seed = MCAINT_SEED_DEFAULT;
  ctx->sparsity = MCAINT_SPARSITY_DEFAULT;
}

void INTERFLOP_MCAINT_API(pre_init)(interflop_panic_t panic, File *stream,
                                    void **context) {
  interflop_set_handler("panic", panic);
  _mcaint_check_stdlib();

  /* Initialize the logger */
  logger_init(panic, stream, backend_name);

  /* allocate the context */
  _mcaint_alloc_context(context);
  _mcaint_init_context((mcaint_context_t *)*context);
}

static struct argp_option options[] = {
    {key_prec_b32_str, KEY_PREC_B32, "PRECISION", 0,
     "select precision for binary32 (PRECISION > 0)", 0},
    {key_prec_b64_str, KEY_PREC_B64, "PRECISION", 0,
     "select precision for binary64 (PRECISION > 0)", 0},
    {key_mode_str, KEY_MODE, "MODE", 0,
     "select MCA mode among {ieee, mca, pb, rr}", 0},
    {key_seed_str, KEY_SEED, "SEED", 0, "fix the random generator seed", 0},
    {key_daz_str, KEY_DAZ, 0, 0,
     "denormals-are-zero: sets denormals inputs to zero", 0},
    {key_ftz_str, KEY_FTZ, 0, 0, "flush-to-zero: sets denormal output to zero",
     0},
    {key_sparsity_str, KEY_SPARSITY, "SPARSITY", 0,
     "one in {sparsity} operations will be perturbed. 0 < sparsity <= 1.", 0},
    {0}};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  mcaint_context_t *ctx = (mcaint_context_t *)state->input;
  char *endptr;
  int val = -1;
  int error = 0;
  float sparsity = -1;
  uint64_t seed = -1;
  switch (key) {
  case KEY_PREC_B32:
    /* precision for binary32 */
    error = 0;
    val = interflop_strtol(arg, &endptr, &error);
    if (error != 0 || val != MCAINT_PRECISION_BINARY32_DEFAULT) {
      logger_error("--%s invalid value provided, MCA integer does not support "
                   "custom precisions",
                   key_prec_b32_str);
    }
    _set_mcaint_precision_binary32(val, ctx);
    break;
  case KEY_PREC_B64:
    /* precision for binary64 */
    error = 0;
    val = interflop_strtol(arg, &endptr, &error);
    if (error != 0 || val != MCAINT_PRECISION_BINARY64_DEFAULT) {
      logger_error("--%s invalid value provided, MCA integer does not support "
                   "custom precisions",
                   key_prec_b64_str);
    }
    _set_mcaint_precision_binary64(val, ctx);
    break;
  case KEY_MODE:
    /* mca mode */
    if (interflop_strcasecmp(MCAINT_MODE_STR[mcaint_mode_ieee], arg) == 0) {
      _set_mcaint_mode(mcaint_mode_ieee, ctx);
    } else if (interflop_strcasecmp(MCAINT_MODE_STR[mcaint_mode_mca], arg) ==
               0) {
      _set_mcaint_mode(mcaint_mode_mca, ctx);
    } else if (interflop_strcasecmp(MCAINT_MODE_STR[mcaint_mode_pb], arg) ==
               0) {
      _set_mcaint_mode(mcaint_mode_pb, ctx);
    } else if (interflop_strcasecmp(MCAINT_MODE_STR[mcaint_mode_rr], arg) ==
               0) {
      _set_mcaint_mode(mcaint_mode_rr, ctx);
    } else {
      logger_error("--%s invalid value provided, must be one of: "
                   "{ieee, mca, pb, rr}.",
                   key_mode_str);
    }
    break;
  case KEY_SEED:
    /* seed */
    error = 0;
    seed = interflop_strtol(arg, &endptr, &error);
    if (error != 0) {
      logger_error("--%s invalid value provided, must be an integer",
                   key_seed_str);
    }
    _set_mcaint_seed(seed, ctx);
    break;
  case KEY_DAZ:
    /* denormals-are-zero */
    _set_mcaint_daz(true, ctx);
    break;
  case KEY_FTZ:
    /* flush-to-zero */
    _set_mcaint_ftz(true, ctx);
    break;
  case KEY_SPARSITY:
    /* sparse perturbations */
    error = 0;
    sparsity = interflop_strtod(arg, &endptr, &error);
    if (ctx->sparsity <= 0) {
      error = 1;
    }
    if (error != 0) {
      logger_error("--%s invalid value provided, must be positive",
                   key_sparsity_str);
    }
    _set_mcaint_sparsity(sparsity, ctx);
    break;
  default:
    return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

static struct argp argp = {options, parse_opt, "", "", NULL, NULL, NULL};

void INTERFLOP_MCAINT_API(cli)(int argc, char **argv, void *context) {
  /* parse backend arguments */
  mcaint_context_t *ctx = (mcaint_context_t *)context;
  if (interflop_argp_parse != NULL) {
    interflop_argp_parse(&argp, argc, argv, 0, 0, ctx);
  } else {
    interflop_panic("Interflop backend error: argp_parse not implemented\n"
                    "Provide implementation or use interflop_configure to "
                    "configure the backend\n");
  }
}

void INTERFLOP_MCAINT_API(configure)(void *configure, void *context) {
  mcaint_context_t *ctx = (mcaint_context_t *)context;
  mcaint_conf_t *conf = (mcaint_conf_t *)configure;
  _set_mcaint_seed(conf->seed, ctx);
  _set_mcaint_sparsity(conf->sparsity, ctx);
  _set_mcaint_precision_binary32(conf->precision_binary32, ctx);
  _set_mcaint_precision_binary64(conf->precision_binary64, ctx);
  _set_mcaint_mode(conf->mode, ctx);
  _set_mcaint_error_mode(conf->err_mode, ctx);
  if (conf->err_mode == mcaint_err_mode_abs ||
      conf->err_mode == mcaint_err_mode_all) {
    _set_mcaint_max_abs_err_exp(conf->max_abs_err_exponent, ctx);
  }
  _set_mcaint_daz(conf->daz, ctx);
  _set_mcaint_ftz(conf->ftz, ctx);
}

static void print_information_header(void *context) {
  mcaint_context_t *ctx = (mcaint_context_t *)context;

  logger_info("load backend with:\n");
  logger_info("%s = %d\n", key_prec_b32_str, ctx->binary32_precision);
  logger_info("%s = %d\n", key_prec_b64_str, ctx->binary64_precision);
  logger_info("%s = %s\n", key_mode_str, MCAINT_MODE_STR[ctx->mode]);
  logger_info("%s = %s\n", key_err_mode_str, _get_mcaint_error_mode_str(ctx));
  logger_info("%s = %d\n", key_err_exp_str, ctx->absErr_exp);
  logger_info("%s = %s\n", key_daz_str, ctx->daz ? "true" : "false");
  logger_info("%s = %s\n", key_ftz_str, ctx->ftz ? "true" : "false");
  logger_info("%s = %f\n", key_sparsity_str, ctx->sparsity);
  logger_info("%s = %lu\n", key_seed_str, ctx->seed);
}

struct interflop_backend_interface_t INTERFLOP_MCAINT_API(init)(void *context) {

  mcaint_context_t *ctx = (mcaint_context_t *)context;

  print_information_header(ctx);

  struct interflop_backend_interface_t interflop_backend_mcaint = {
    interflop_add_float : INTERFLOP_MCAINT_API(add_float),
    interflop_sub_float : INTERFLOP_MCAINT_API(sub_float),
    interflop_mul_float : INTERFLOP_MCAINT_API(mul_float),
    interflop_div_float : INTERFLOP_MCAINT_API(div_float),
    interflop_cmp_float : NULL,
    interflop_add_double : INTERFLOP_MCAINT_API(add_double),
    interflop_sub_double : INTERFLOP_MCAINT_API(sub_double),
    interflop_mul_double : INTERFLOP_MCAINT_API(mul_double),
    interflop_div_double : INTERFLOP_MCAINT_API(div_double),
    interflop_cmp_double : NULL,
    interflop_cast_double_to_float : INTERFLOP_MCAINT_API(cast_double_to_float),
    interflop_fma_float : INTERFLOP_MCAINT_API(fma_float),
    interflop_fma_double : INTERFLOP_MCAINT_API(fma_double),
    interflop_enter_function : NULL,
    interflop_exit_function : NULL,
    interflop_user_call : NULL,
    interflop_finalize : NULL
  };

  /* The seed for the RNG is initialized upon the first request for a random
     number */
  _init_rng_state_struct(&rng_state, ctx->choose_seed, ctx->seed, false);

  return interflop_backend_mcaint;
}

struct interflop_backend_interface_t interflop_init(void *context)
    __attribute__((weak, alias("interflop_mcaint_init")));

void interflop_pre_init(interflop_panic_t panic, File *stream, void **context)
    __attribute__((weak, alias("interflop_mcaint_pre_init")));

void interflop_cli(int argc, char **argv, void *context)
    __attribute__((weak, alias("interflop_mcaint_cli")));
