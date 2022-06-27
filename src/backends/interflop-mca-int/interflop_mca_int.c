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
 *  Copyright (c) 2019-2022                                                  *\
 *     Verificarlo Contributors                                              *\
 *                                                                           *\
 ****************************************************************************/

#include <argp.h>
#include <err.h>
#include <errno.h>
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

#include "../../common/float_const.h"
#include "../../common/float_struct.h"
#include "../../common/float_utils.h"
#include "../../common/interflop.h"
#include "../../common/logger.h"
#include "../../common/options.h"

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

typedef struct {
  bool relErr;
  bool absErr;
  int absErr_exp;
  bool choose_seed;
  uint64_t seed;
  bool daz;
  bool ftz;
  float sparsity;
} t_context;

/* define the available MCA modes of operation */
typedef enum {
  mcamode_ieee,
  mcamode_mca,
  mcamode_pb,
  mcamode_rr,
  _mcamode_end_
} mcamode;

static const char *MCA_MODE_STR[] = {"ieee", "mca", "pb", "rr"};

/* define the available error modes */
typedef enum {
  mca_err_mode_rel,
  mca_err_mode_abs,
  mca_err_mode_all
} mca_err_mode;

static const char *MCA_ERR_MODE_STR[] = {"rel", "abs", "all"};

/* define default environment variables and default parameters */
#define MCA_PRECISION_BINARY32_MIN 1
#define MCA_PRECISION_BINARY64_MIN 1
#define MCA_PRECISION_BINARY32_MAX 53
#define MCA_PRECISION_BINARY64_MAX 112
#define MCA_PRECISION_BINARY32_DEFAULT 24
#define MCA_PRECISION_BINARY64_DEFAULT 53
#define MCA_MODE_DEFAULT mcamode_mca

static mcamode MCALIB_MODE = MCA_MODE_DEFAULT;
static int MCALIB_BINARY32_T = MCA_PRECISION_BINARY32_DEFAULT;
static int MCALIB_BINARY64_T = MCA_PRECISION_BINARY64_DEFAULT;

/* possible operations values */
typedef enum {
  mca_add = '+',
  mca_sub = '-',
  mca_mul = '*',
  mca_div = '/'
} mca_operations;

static float _mca_binary32_binary_op(float a, float b, const mca_operations op,
                                     void *context);
static double _mca_binary64_binary_op(double a, double b,
                                      const mca_operations op, void *context);

/******************** MCA CONTROL FUNCTIONS *******************
 * The following functions are used to set virtual precision and
 * MCA mode of operation.
 ***************************************************************/

/* Set the mca mode */
static void _set_mca_mode(const mcamode mode) {
  if (mode >= _mcamode_end_) {
    logger_error("--%s invalid value provided, must be one of: "
                 "{ieee, mca, pb, rr}.",
                 key_mode_str);
  }
  MCALIB_MODE = mode;
}

/* Set the virtual precision for binary32 */
static void _set_mca_precision_binary32(const int precision) {
  _set_precision(MCA, precision, &MCALIB_BINARY32_T, (float)0);
}

/* Set the virtual precision for binary64 */
static void _set_mca_precision_binary64(const int precision) {
  _set_precision(MCA, precision, &MCALIB_BINARY64_T, (double)0);
}

/******************** MCA RANDOM FUNCTIONS ********************
 * The following functions are used to calculate the random
 * perturbations used for MCA
 ***************************************************************/

/* global thread id access lock */
static pthread_mutex_t global_tid_lock = PTHREAD_MUTEX_INITIALIZER;
/* global thread identifier */
static unsigned long long int global_tid = 0;

/* helper data structure to centralize the data used for random number
 * generation */
static __thread rng_state_t rng_state;

/* noise = rand * 2^(exp) */
/* We can skip special cases since we never meet them */
/* Since we have exponent of float values, the result */
/* is comprised between: */
/* 127+127 = 254 < DOUBLE_EXP_MAX (1023)  */
/* -126-24+-126-24 = -300 > DOUBLE_EXP_MIN (-1022) */
static inline void _noise_binary64(double *x, const int exp,
                                   rng_state_t *rng_state) {
  int64_t noise;
  uint32_t shift;

  // Convert preserving-bytes double to int64_t
  int64_t x_s64 = *(int64_t *)(x);

  // amount by which to shift the noise term sign (1) + exp (11) + noise
  // exponent
  shift = 1 + DOUBLE_EXP_SIZE - exp;

  // noise is a signed integer so the noise is centered around 0
  // only 32 bits of noise are used, they are left aligned in a signed 64 bit
  noise = ((uint64_t)_get_rand_uint32(rng_state, &global_tid_lock, &global_tid)
           << 32);

  // right shift the noise to the correct magnitude, this is a arithmetic shift
  // and sign bit will be extended
  noise = noise >> shift;

  // Add the noise to the x value
  x_s64 += noise;

  // Convert back to double
  *x = *(double *)(&x_s64);
}

/* noise = rand * 2^(exp) */
/* We can skip special cases since we never meet them */
/* Since we have exponent of double values, the result */
/* is comprised between: */
/* 1023+1023 = 2046 < QUAD_EXP_MAX (16383)  */
/* -1022-53+-1022-53 = -2200 > QUAD_EXP_MIN (-16382) */
static void _noise_binary128(__float128 *x, const int exp,
                             rng_state_t *rng_state) {

  // Convert preserving-bytes __float128 to __int128
  __int128 x_s128 = *(__int128 *)(x);

  // amount by which to shift the noise term sign (1) + exp (15) + noise
  // exponent
  uint32_t shift = 1 + QUAD_EXP_SIZE - exp;

  // Generate 128 signed noise
  // only 64 bits of noise are used, they are left aligned in a signed 64 bit
  int64_t noise_high =
      _get_rand_uint64(rng_state, &global_tid_lock, &global_tid);
  __int128 noise = noise_high;
  noise <<= 64;

  // right shift the noise to the correct magnitude, this is a arithmetic shift
  // and sign bit will be extended
  noise = noise >> shift;

  // Add the noise
  x_s128 += noise;

  // Convert back to __float128
  *x = *(__float128 *)(&x_s128);
}

/* Macro function for checking if the value X must be noised */
#define _MUST_NOT_BE_NOISED(X, VIRTUAL_PRECISION)                                                            \
  /* if mode ieee, do not introduce noise */                                                                 \
  (MCALIB_MODE ==                                                                                            \
   mcamode_ieee) || /* Check that we are not in a special case */                                            \
      (FPCLASSIFY(X) != FP_NORMAL && FPCLASSIFY(X) != FP_SUBNORMAL) ||                                       \
      /* In RR if the number is representable in current virtual precision, */ /* do not add any noise if */ \
      (MCALIB_MODE == mcamode_rr && _IS_REPRESENTABLE(X, VIRTUAL_PRECISION))

/* Generic function for computing the mca noise */
#define _NOISE(X, EXP, RNG_STATE)                                              \
  _Generic(*X, double                                                          \
           : _noise_binary64, __float128                                       \
           : _noise_binary128)(X, EXP, RNG_STATE)

/* Macro function that adds mca noise to X
   according to the virtual_precision VIRTUAL_PRECISION */
#define _INEXACT(X, VIRTUAL_PRECISION, CTX, RNG_STATE)                         \
  {                                                                            \
    t_context *TMP_CTX = (t_context *)CTX;                                     \
    _init_rng_state_struct(&RNG_STATE, TMP_CTX->choose_seed,                   \
                           (unsigned long long)(TMP_CTX->seed), false);        \
    if (_MUST_NOT_BE_NOISED(*X, VIRTUAL_PRECISION)) {                          \
      return;                                                                  \
    } else if (_mca_skip_eval(TMP_CTX->sparsity, &(RNG_STATE),                 \
                              &global_tid_lock, &global_tid)) {                \
      return;                                                                  \
    } else {                                                                   \
      const int32_t e_n_rel = -(VIRTUAL_PRECISION - 1);                        \
      _NOISE(X, e_n_rel, &RNG_STATE);                                          \
    }                                                                          \
  }

/* Adds the mca noise to da */
static void _mca_inexact_binary64(double *da, void *context) {
  _INEXACT(da, MCALIB_BINARY32_T, context, rng_state);
}

/* Adds the mca noise to qa */
static void _mca_inexact_binary128(__float128 *qa, void *context) {
  _INEXACT(qa, MCALIB_BINARY64_T, context, rng_state);
}

/* Generic functions that adds noise to A */
/* The function is choosen depending on the type of X  */
#define _INEXACT_BINARYN(X, A, CTX)                                            \
  _Generic(X, double                                                           \
           : _mca_inexact_binary64, __float128                                 \
           : _mca_inexact_binary128)(A, CTX)

/******************** MCA ARITHMETIC FUNCTIONS ********************
 * The following set of functions perform the MCA operation. Operands
 * are first converted to quad  format (GCC), inbound and outbound
 * perturbations are applied using the _mca_inexact function, and the
 * result converted to the original format for return
 *******************************************************************/

/* perform_bin_op: applies the binary operator (op) to (a) and (b) */
/* and stores the result in (res) */
#define PERFORM_BIN_OP(OP, RES, A, B)                                          \
  switch (OP) {                                                                \
  case mca_add:                                                                \
    RES = (A) + (B);                                                           \
    break;                                                                     \
  case mca_mul:                                                                \
    RES = (A) * (B);                                                           \
    break;                                                                     \
  case mca_sub:                                                                \
    RES = (A) - (B);                                                           \
    break;                                                                     \
  case mca_div:                                                                \
    RES = (A) / (B);                                                           \
    break;                                                                     \
  default:                                                                     \
    logger_error("invalid operator %c", OP);                                   \
  };

/* Generic macro function that returns mca(A OP B) */
/* Functions are determined according to the type of X */
#define _MCA_BINARY_OP(A, B, OP, CTX, X)                                       \
  do {                                                                         \
    typeof(X) _A = A;                                                          \
    typeof(X) _B = B;                                                          \
    typeof(X) _RES = 0;                                                        \
    if (((t_context *)CTX)->daz) {                                             \
      _A = DAZ(A);                                                             \
      _B = DAZ(B);                                                             \
    }                                                                          \
    if (MCALIB_MODE == mcamode_pb || MCALIB_MODE == mcamode_mca) {             \
      _INEXACT_BINARYN(X, &_A, CTX);                                           \
      _INEXACT_BINARYN(X, &_B, CTX);                                           \
    }                                                                          \
    PERFORM_BIN_OP(OP, _RES, _A, _B);                                          \
    if (MCALIB_MODE == mcamode_rr || MCALIB_MODE == mcamode_mca) {             \
      _INEXACT_BINARYN(X, &_RES, CTX);                                         \
    }                                                                          \
    if (((t_context *)CTX)->ftz) {                                             \
      _RES = FTZ((typeof(A))_RES);                                             \
    }                                                                          \
    return (typeof(A))(_RES);                                                  \
  } while (0);

/* Performs mca(a dop b) where a and b are binary32 values */
/* Intermediate computations are performed with binary64 */
inline float _mca_binary32_binary_op(const float a, const float b,
                                     const mca_operations dop, void *context) {
  _MCA_BINARY_OP(a, b, dop, context, (double)0);
}

/* Performs mca(a qop b) where a and b are binary64 values */
/* Intermediate computations are performed with binary128 */
inline double _mca_binary64_binary_op(const double a, const double b,
                                      const mca_operations qop, void *context) {
  _MCA_BINARY_OP(a, b, qop, context, (__float128)0);
}

/************************* FPHOOKS FUNCTIONS *************************
 * These functions correspond to those inserted into the source code
 * during source to source compilation and are replacement to floating
 * point operators
 **********************************************************************/

_INTERFLOP_OP_CALL(float, add, mca_add, _mca_binary32_binary_op)

_INTERFLOP_OP_CALL(float, sub, mca_sub, _mca_binary32_binary_op)

_INTERFLOP_OP_CALL(float, mul, mca_mul, _mca_binary32_binary_op)

_INTERFLOP_OP_CALL(float, div, mca_div, _mca_binary32_binary_op)

_INTERFLOP_OP_CALL(double, add, mca_add, _mca_binary64_binary_op)

_INTERFLOP_OP_CALL(double, sub, mca_sub, _mca_binary64_binary_op)

_INTERFLOP_OP_CALL(double, mul, mca_mul, _mca_binary64_binary_op)

_INTERFLOP_OP_CALL(double, div, mca_div, _mca_binary64_binary_op)

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

error_t parse_opt(int key, char *arg, struct argp_state *state) {
  t_context *ctx = (t_context *)state->input;
  char *endptr;
  int val = -1;
  switch (key) {
  case KEY_PREC_B32:
    /* precision for binary32 */
    errno = 0;
    val = strtol(arg, &endptr, 10);
    if (errno != 0 || val != MCA_PRECISION_BINARY32_DEFAULT) {
      logger_error("--%s invalid value provided, MCA integer does not support "
                   "custom precisions",
                   key_prec_b32_str);
    }
    break;
  case KEY_PREC_B64:
    /* precision for binary64 */
    errno = 0;
    val = strtol(arg, &endptr, 10);
    if (errno != 0 || val != MCA_PRECISION_BINARY64_DEFAULT) {
      logger_error("--%s invalid value provided, MCA integer does not support "
                   "custom precisions",
                   key_prec_b64_str);
    }
    break;
  case KEY_MODE:
    /* mca mode */
    if (strcasecmp(MCA_MODE_STR[mcamode_ieee], arg) == 0) {
      _set_mca_mode(mcamode_ieee);
    } else if (strcasecmp(MCA_MODE_STR[mcamode_mca], arg) == 0) {
      _set_mca_mode(mcamode_mca);
    } else if (strcasecmp(MCA_MODE_STR[mcamode_pb], arg) == 0) {
      _set_mca_mode(mcamode_pb);
    } else if (strcasecmp(MCA_MODE_STR[mcamode_rr], arg) == 0) {
      _set_mca_mode(mcamode_rr);
    } else {
      logger_error("--%s invalid value provided, must be one of: "
                   "{ieee, mca, pb, rr}.",
                   key_mode_str);
    }
    break;
  case KEY_SEED:
    /* seed */
    errno = 0;
    ctx->choose_seed = true;
    ctx->seed = strtoull(arg, &endptr, 10);
    if (errno != 0) {
      logger_error("--%s invalid value provided, must be an integer",
                   key_seed_str);
    }
    break;
  case KEY_DAZ:
    /* denormals-are-zero */
    ctx->daz = true;
    break;
  case KEY_FTZ:
    /* flush-to-zero */
    ctx->ftz = true;
    break;
  case KEY_SPARSITY:
    /* sparse perturbations */
    errno = 0;
    ctx->sparsity = strtof(arg, &endptr);
    if (ctx->sparsity <= 0) {
      errno = 1;
    }
    if (errno != 0) {
      logger_error("--%s invalid value provided, must be positive",
                   key_sparsity_str);
    }
    break;
  default:
    return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

struct argp argp = {options, parse_opt, "", "", NULL, NULL, NULL};

void init_context(t_context *ctx) {
  ctx->relErr = true;
  ctx->absErr = false;
  ctx->absErr_exp = 112;
  ctx->choose_seed = false;
  ctx->daz = false;
  ctx->ftz = false;
  ctx->seed = 0ULL;
  ctx->sparsity = 1.0f;
}

void print_information_header(void *context) {
  t_context *ctx = (t_context *)context;

  logger_info("load backend with "
              "%s = %d, "
              "%s = %d, "
              "%s = %s, "
              "%s = %s, "
              "%s = %d, "
              "%s = %s, "
              "%s = %s and "
              "%s = %f"
              "\n",
              key_prec_b32_str, MCALIB_BINARY32_T, key_prec_b64_str,
              MCALIB_BINARY64_T, key_mode_str, MCA_MODE_STR[MCALIB_MODE],
              key_err_mode_str,
              (ctx->relErr && !ctx->absErr)
                  ? MCA_ERR_MODE_STR[mca_err_mode_rel]
                  : (!ctx->relErr && ctx->absErr)
                        ? MCA_ERR_MODE_STR[mca_err_mode_abs]
                        : (ctx->relErr && ctx->absErr)
                              ? MCA_ERR_MODE_STR[mca_err_mode_all]
                              : MCA_ERR_MODE_STR[mca_err_mode_rel],
              key_err_exp_str, (ctx->absErr_exp), key_daz_str,
              ctx->daz ? "true" : "false", key_ftz_str,
              ctx->ftz ? "true" : "false", key_sparsity_str, ctx->sparsity);
}

struct interflop_backend_interface_t interflop_init(int argc, char **argv,
                                                    void **context) {

  /* Initialize the logger */
  logger_init();

  /* Mca integer backend only supports default precision
     and relative error mode */
  _set_mca_precision_binary32(MCA_PRECISION_BINARY32_DEFAULT);
  _set_mca_precision_binary64(MCA_PRECISION_BINARY64_DEFAULT);
  _set_mca_mode(MCA_MODE_DEFAULT);

  t_context *ctx = malloc(sizeof(t_context));
  *context = ctx;
  init_context(ctx);

  /* Parse backend arguments */
  argp_parse(&argp, argc, argv, 0, 0, ctx);

  print_information_header(ctx);

  struct interflop_backend_interface_t interflop_backend_mca = {
      _interflop_add_float,
      _interflop_sub_float,
      _interflop_mul_float,
      _interflop_div_float,
      NULL,
      _interflop_add_double,
      _interflop_sub_double,
      _interflop_mul_double,
      _interflop_div_double,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL};

  /* The seed for the RNG is initialized upon the first request for a random
     number */

  _init_rng_state_struct(&rng_state, ctx->choose_seed,
                         (unsigned long long int)(ctx->seed), false);

  return interflop_backend_mca;
}
