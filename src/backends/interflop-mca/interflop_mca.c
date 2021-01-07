/*****************************************************************************
 *                                                                           *
 *  This file is part of Verificarlo.                                        *
 *                                                                           *
 *  Copyright (c) 2015                                                       *
 *     Universite de Versailles St-Quentin-en-Yvelines                       *
 *     CMLA, Ecole Normale Superieure de Cachan                              *
 *  Copyright (c) 2018-2020                                                  *
 *     Verificarlo contributors                                              *
 *     Universite de Versailles St-Quentin-en-Yvelines                       *
 *                                                                           *
 *  Verificarlo is free software: you can redistribute it and/or modify      *
 *  it under the terms of the GNU General Public License as published by     *
 *  the Free Software Foundation, either version 3 of the License, or        *
 *  (at your option) any later version.                                      *
 *                                                                           *
 *  Verificarlo is distributed in the hope that it will be useful,           *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *  GNU General Public License for more details.                             *
 *                                                                           *
 *  You should have received a copy of the GNU General Public License        *
 *  along with Verificarlo.  If not, see <http://www.gnu.org/licenses/>.     *
 *                                                                           *
 *****************************************************************************/

// Changelog:
//
// 2015-05-20 replace random number generator with TinyMT64. This
// provides a reentrant, independent generator of better quality than
// the one provided in libc.
//
// 2015-10-11 New version based on quad floating point type to replace MPFR
// until
// required MCA precision is lower than quad mantissa divided by 2, i.e. 56 bits
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
// and binary64. Uses the binary128 structur for easily manipulating bits
// through bitfields. Removes useless specials cases in qnoise and pow2d.
// Change return type from int to void for some functions and uses instead
// errx and warnx for handling errors.
//
// 2020-02-26 Factorize _inexact function into the _INEXACT macro function.
// Use variables for options name instead of hardcoded one.
// Add DAZ/FTZ support.

#include <argp.h>
#include <err.h>
#include <errno.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/time.h>
#include <unistd.h>

#include "../../common/float_const.h"
#include "../../common/float_struct.h"
#include "../../common/float_utils.h"
#include "../../common/interflop.h"
#include "../../common/logger.h"
#include "../../common/options.h"
#include "../../common/tinymt64.h"

typedef enum {
  KEY_PREC_B32,
  KEY_PREC_B64,
  KEY_ERR_EXP,
  KEY_MODE = 'm',
  KEY_ERR_MODE = 'e',
  KEY_SEED = 's',
  KEY_DAZ = 'd',
  KEY_FTZ = 'f'
} key_args;

static const char key_prec_b32_str[] = "precision-binary32";
static const char key_prec_b64_str[] = "precision-binary64";
static const char key_mode_str[] = "mode";
static const char key_err_mode_str[] = "error-mode";
static const char key_err_exp_str[] = "max-abs-error-exponent";
static const char key_seed_str[] = "seed";
static const char key_daz_str[] = "daz";
static const char key_ftz_str[] = "ftz";

typedef struct {
  bool relErr;
  bool absErr;
  int absErr_exp;
  bool choose_seed;
  uint64_t seed;
  bool daz;
  bool ftz;
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

/* random generator internal state */
static tinymt64_t random_state;

static double _mca_rand(void) {
  /* Returns a random double in the (0,1) open interval */
  return tinymt64_generate_doubleOO(&random_state);
}

/* noise = rand * 2^(exp) */
/* We can skip special cases since we never met them */
/* Since we have exponent of float values, the result */
/* is comprised between: */
/* 127+127 = 254 < DOUBLE_EXP_MAX (1023)  */
/* -126-24+-126-24 = -300 > DOUBLE_EXP_MIN (-1022) */
static inline double _noise_binary64(const int exp) {
  const double d_rand = (_mca_rand() - 0.5);
  return _fast_pow2_binary64(exp) * d_rand;
}

/* noise = rand * 2^(exp) */
/* We can skip special cases since we never met them */
/* Since we have exponent of double values, the result */
/* is comprised between: */
/* 1023+1023 = 2046 < QUAD_EXP_MAX (16383)  */
/* -1022-53+-1022-53 = -2200 > QUAD_EXP_MIN (-16382) */
static __float128 _noise_binary128(const int exp) {
  /* random number in (-0.5, 0.5) */
  const __float128 noise = (__float128)_mca_rand() - 0.5Q;
  return _fast_pow2_binary128(exp) * noise;
}

/* Macro function for checking if the value X must be noised */
#define _MUST_NOT_BE_NOISED(X, VIRTUAL_PRECISION)                              \
  /* if mode ieee, do not introduce noise */                                   \
  (MCALIB_MODE == mcamode_ieee) ||					                                   \
  /* Check that we are not in a special case */				                         \
  (FPCLASSIFY(X) != FP_NORMAL && FPCLASSIFY(X) != FP_SUBNORMAL) ||	           \
  /* In RR if the number is representable in current virtual precision, */     \
  /* do not add any noise if */						                                     \
  (MCALIB_MODE == mcamode_rr && _IS_REPRESENTABLE(X, VIRTUAL_PRECISION))

/* Generic function for computing the mca noise */
#define _NOISE(X, EXP)                                                         \
  _Generic(X, double : _noise_binary64, __float128 : _noise_binary128)(EXP)

/* Macro function that adds mca noise to X
   according to the virtual_precision VIRTUAL_PRECISION */
#define _INEXACT(X, VIRTUAL_PRECISION, CTX)                                    \
  {                                                                            \
    if (_MUST_NOT_BE_NOISED(*X, VIRTUAL_PRECISION)) {                          \
      return;                                                                  \
    } else {                                                                   \
      if (((t_context *)CTX)->relErr) {                                        \
        const int32_t e_a = GET_EXP_FLT(*X);                                   \
        const int32_t e_n_rel = e_a - (VIRTUAL_PRECISION - 1);                 \
        const typeof(*X) noise_rel = _NOISE(*X, e_n_rel);                      \
        *X = *X + noise_rel;                                                   \
      }                                                                        \
      if (((t_context *)CTX)->absErr) {                                        \
        const int32_t e_n_abs = ((t_context *)CTX)->absErr_exp;                \
        const typeof(*X) noise_abs = _NOISE(*X, e_n_abs);                      \
        *X = *X + noise_abs;                                                   \
      }                                                                        \
    }                                                                          \
  }

/* Adds the mca noise to da */
static void _mca_inexact_binary64(double *da, void *context) {
  _INEXACT(da, MCALIB_BINARY32_T, context);
}

/* Adds the mca noise to qa */
static void _mca_inexact_binary128(__float128 *qa, void *context) {
  _INEXACT(qa, MCALIB_BINARY64_T, context);
}

/* Generic functions that adds noise to A */
/* The function is choosen depending on the type of X  */
#define _INEXACT_BINARYN(X, A, CTX)                                            \
  _Generic(X, double                                                           \
           : _mca_inexact_binary64, __float128                                 \
           : _mca_inexact_binary128)(A, CTX)

/* Set the mca seed */
static void _set_mca_seed(const bool choose_seed, const uint64_t seed) {
  _set_seed_default(&random_state, choose_seed, seed);
}

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

static void _interflop_add_float(float a, float b, float *c, void *context) {
  *c = _mca_binary32_binary_op(a, b, mca_add, context);
}

static void _interflop_sub_float(float a, float b, float *c, void *context) {
  *c = _mca_binary32_binary_op(a, b, mca_sub, context);
}

static void _interflop_mul_float(float a, float b, float *c, void *context) {
  *c = _mca_binary32_binary_op(a, b, mca_mul, context);
}

static void _interflop_div_float(float a, float b, float *c, void *context) {
  *c = _mca_binary32_binary_op(a, b, mca_div, context);
}

static void _interflop_add_double(double a, double b, double *c,
                                  void *context) {
  *c = _mca_binary64_binary_op(a, b, mca_add, context);
}

static void _interflop_sub_double(double a, double b, double *c,
                                  void *context) {
  *c = _mca_binary64_binary_op(a, b, mca_sub, context);
}

static void _interflop_mul_double(double a, double b, double *c,
                                  void *context) {
  *c = _mca_binary64_binary_op(a, b, mca_mul, context);
}

static void _interflop_div_double(double a, double b, double *c,
                                  void *context) {
  *c = _mca_binary64_binary_op(a, b, mca_div, context);
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
    if (errno != 0 || val <= 0) {
      logger_error("--%s invalid value provided, must be a positive integer",
                   key_prec_b32_str);
    } else {
      _set_mca_precision_binary32(val);
    }
    break;
  case KEY_PREC_B64:
    /* precision for binary64 */
    errno = 0;
    val = strtol(arg, &endptr, 10);
    if (errno != 0 || val <= 0) {
      logger_error("--%s invalid value provided, must be a positive integer",
                   key_prec_b64_str);
    } else {
      _set_mca_precision_binary64(val);
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
  case KEY_ERR_MODE:
    /* mca error mode */
    if (strcasecmp(MCA_ERR_MODE_STR[mca_err_mode_rel], arg) == 0) {
      ctx->relErr = true;
      ctx->absErr = false;
    } else if (strcasecmp(MCA_ERR_MODE_STR[mca_err_mode_abs], arg) == 0) {
      ctx->relErr = false;
      ctx->absErr = true;
    } else if (strcasecmp(MCA_ERR_MODE_STR[mca_err_mode_all], arg) == 0) {
      ctx->relErr = true;
      ctx->absErr = true;
    } else {
      logger_error("--%s invalid value provided, must be one of: "
                   "{rel, abs, all}.",
                   key_err_mode_str);
    }
    break;
  case KEY_ERR_EXP:
    /* exponent of the maximum absolute error */
    errno = 0;
    ctx->absErr_exp = strtol(arg, &endptr, 10);
    if (errno != 0) {
      logger_error("--%s invalid value provided, must be an integer",
                   key_err_exp_str);
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
}

void print_information_header(void *context) {
  t_context *ctx = (t_context *)context;

  logger_info(
      "load backend with "
      "%s = %d, "
      "%s = %d, "
      "%s = %s, "
      "%s = %s, "
      "%s = %d, "
      "%s = %s and "
      "%s = %s"
      "\n",
      key_prec_b32_str, MCALIB_BINARY32_T, key_prec_b64_str, MCALIB_BINARY64_T,
      key_mode_str, MCA_MODE_STR[MCALIB_MODE], key_err_mode_str,
      (ctx->relErr && !ctx->absErr)
          ? MCA_ERR_MODE_STR[mca_err_mode_rel]
          : (!ctx->relErr && ctx->absErr)
                ? MCA_ERR_MODE_STR[mca_err_mode_abs]
                : (ctx->relErr && ctx->absErr)
                      ? MCA_ERR_MODE_STR[mca_err_mode_all]
                      : MCA_ERR_MODE_STR[mca_err_mode_rel],
      key_err_exp_str, (ctx->absErr_exp), key_daz_str,
      ctx->daz ? "true" : "false", key_ftz_str, ctx->ftz ? "true" : "false");
}

struct interflop_backend_interface_t interflop_init(int argc, char **argv,
                                                    void **context) {

  /* Initialize the logger */
  logger_init();

  _set_mca_precision_binary32(MCA_PRECISION_BINARY32_DEFAULT);
  _set_mca_precision_binary64(MCA_PRECISION_BINARY64_DEFAULT);
  _set_mca_mode(MCA_MODE_DEFAULT);

  t_context *ctx = malloc(sizeof(t_context));
  *context = ctx;
  init_context(ctx);

  /* parse backend arguments */
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
      NULL};

  /* Initialize the seed */
  _set_mca_seed(ctx->choose_seed, ctx->seed);

  return interflop_backend_mca;
}
