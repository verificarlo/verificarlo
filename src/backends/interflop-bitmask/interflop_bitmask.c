/*****************************************************************************
 *                                                                           *
 *  This file is part of Verificarlo.                                        *
 *                                                                           *
 *  Copyright (c) 2015                                                       *
 *     Universite de Versailles St-Quentin-en-Yvelines                       *
 *     CMLA, Ecole Normale Superieure de Cachan                              *
 *  Copyright (c) 2018-2020                                                  *
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

#include <argp.h>
#include <err.h>
#include <errno.h>
#include <math.h>
#include <mpfr.h>
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
#include "../../common/generic_builtin.h"
#include "../../common/interflop.h"
#include "../../common/logger.h"
#include "../../common/options.h"
#include "../../common/tinymt64.h"

typedef enum {
  KEY_PREC_B32,
  KEY_PREC_B64,
  KEY_MODE = 'm',
  KEY_OPERATOR = 'o',
  KEY_SEED = 's',
  KEY_DAZ = 'd',
  KEY_FTZ = 'f'
} key_args;

static const char key_prec_b32_str[] = "precision-binary32";
static const char key_prec_b64_str[] = "precision-binary64";
static const char key_mode_str[] = "mode";
static const char key_operator_str[] = "operator";
static const char key_seed_str[] = "seed";
static const char key_daz_str[] = "daz";
static const char key_ftz_str[] = "ftz";

typedef struct {
  bool choose_seed;
  uint64_t seed;
  bool daz;
  bool ftz;
} t_context;

/* define the available BITMASK modes of operation */
typedef enum {
  bitmask_mode_ieee,
  bitmask_mode_full,
  bitmask_mode_ib,
  bitmask_mode_ob,
  _bitmask_mode_end_
} bitmask_mode;

/* string name of the bitmask modes */
static const char *BITMASK_MODE_STR[] = {"ieee", "full", "ib", "ob"};

/* define the available BITMASK */
typedef enum {
  bitmask_operator_zero,
  bitmask_operator_one,
  bitmask_operator_rand,
  _bitmask_operator_end_
} bitmask_operator;

/* string name of the bitmask */
static const char *BITMASK_OPERATOR_STR[] = {"zero", "one", "rand"};

/* define default environment variables and default parameters */
#define BITMASK_PRECISION_BINARY32_MIN 1
#define BITMASK_PRECISION_BINARY64_MIN 1
#define BITMASK_PRECISION_BINARY32_MAX 23
#define BITMASK_PRECISION_BINARY64_MAX 52
#define BITMASK_PRECISION_BINARY32_DEFAULT 23
#define BITMASK_PRECISION_BINARY64_DEFAULT 52
#define BITMASK_OPERATOR_DEFAULT bitmask_operator_zero
#define BITMASK_MODE_DEFAULT bitmask_mode_ob

static int BITMASKLIB_MODE = BITMASK_MODE_DEFAULT;
static int BITMASKLIB_OPERATOR = BITMASK_OPERATOR_DEFAULT;
static int BITMASKLIB_BINARY32_T = BITMASK_PRECISION_BINARY64_DEFAULT;
static int BITMASKLIB_BINARY64_T = BITMASK_PRECISION_BINARY32_DEFAULT;

#define GET_BINARYN_T(X)                                                       \
  _Generic(X, float : BITMASKLIB_BINARY32_T, double : BITMASKLIB_BINARY64_T)

/* possible op values */
typedef enum {
  bitmask_add = '+',
  bitmask_sub = '-',
  bitmask_mul = '*',
  bitmask_div = '/'
} bitmask_operations;

static float _bitmask_binary32_binary_op(float a, float b,
                                         const bitmask_operations op,
                                         void *context);
static double _bitmask_binary64_binary_op(double a, double b,
                                          const bitmask_operations op,
                                          void *context);

static uint32_t binary32_bitmask = FLOAT_MASK_ONE;
static uint64_t binary64_bitmask = DOUBLE_MASK_ONE;

#define GET_BITMASK(X)                                                         \
  _Generic(X, \
				float: binary32_bitmask,    \
				double:binary64_bitmask,    \
				float*:&binary32_bitmask,   \
				double*:&binary64_bitmask)

/******************** BITMASK CONTROL FUNCTIONS *******************
 * The following functions are used to set virtual precision and
 * BITMASK mode of operation.
 ***************************************************************/

static void _set_bitmask_mode(const bitmask_mode mode) {
  if (mode >= _bitmask_mode_end_) {
    logger_error("--%s invalid value provided, must be one of: "
                 "{ieee, full, ib, ob}.",
                 key_mode_str);
  }
  BITMASKLIB_MODE = mode;
}

static void _set_bitmask_operator(const bitmask_operator bitmask) {
  if (bitmask > _bitmask_operator_end_) {
    logger_error("--%s invalid value provided, must be one of: "
                 "{zero, one, rand}.",
                 key_operator_str);
  }
  BITMASKLIB_OPERATOR = bitmask;
}

#define _set_bitmask_precision(precision, VIRTUAL_PRECISION, Y, X)             \
  {                                                                            \
    typeof(Y) *bitmask = GET_BITMASK((typeof(X) *)0);                          \
    const int32_t PREC = GET_PREC(X);                                          \
    typeof(Y) MASK_ONE = GET_MASK_ONE(X);                                      \
    *bitmask = (VIRTUAL_PRECISION <= PREC)                                     \
                   ? MASK_ONE << (PREC - VIRTUAL_PRECISION)                    \
                   : MASK_ONE;                                                 \
  }

static void _set_bitmask_precision_binary32(const int precision) {
  _set_precision(BITMASK, precision, &BITMASKLIB_BINARY32_T, (float)0);
  _set_bitmask_precision(precision, BITMASKLIB_BINARY32_T,
                         (typeof(binary32_bitmask))0, (float)0);
}

static void _set_bitmask_precision_binary64(const int precision) {
  _set_precision(BITMASK, precision, &BITMASKLIB_BINARY64_T, (double)0);
  _set_bitmask_precision(precision, BITMASKLIB_BINARY64_T,
                         (typeof(binary64_bitmask))0, (double)0);
}

/******************** BITMASK RANDOM FUNCTIONS ********************
 * The following functions are used to calculate the random bitmask
 ***************************************************************/

/* random generator internal state */
tinymt64_t random_state;

static uint64_t get_random_mask(void) {
  return tinymt64_generate_uint64(&random_state);
}

/* Returns a 32-bits random mask */
static uint32_t get_random_binary32_mask(void) {
  binary64 mask;
  mask.u64 = get_random_mask();
  return mask.u32[0];
}

/* Returns a 64-bits random mask */
static uint64_t get_random_binary64_mask(void) {
  uint64_t mask = get_random_mask();
  return mask;
}

/* Returns a random mask depending on the type of X */
#define GET_RANDOM_MASK(X)                                                     \
  _Generic(X, float                                                            \
           : get_random_binary32_mask, double                                  \
           : get_random_binary64_mask)()

/* Fix the seed of the Random Number Generator */
static void _set_bitmask_seed(const bool choose_seed, const uint64_t seed) {
  _set_seed_default(&random_state, choose_seed, seed);
}

/******************** BITMASK ARITHMETIC FUNCTIONS ********************
 * The following set of functions perform the BITMASK operation. Operands
 * They apply a bitmask to the result
 *******************************************************************/

/* perform_bin_op: applies the binary operator (op) to (a) and (b) */
/* and stores the result in (res) */
#define PERFORM_BIN_OP(OP, RES, A, B)                                          \
  switch (OP) {                                                                \
  case bitmask_add:                                                            \
    RES = (A) + (B);                                                           \
    break;                                                                     \
  case bitmask_mul:                                                            \
    RES = (A) * (B);                                                           \
    break;                                                                     \
  case bitmask_sub:                                                            \
    RES = (A) - (B);                                                           \
    break;                                                                     \
  case bitmask_div:                                                            \
    RES = (A) / (B);                                                           \
    break;                                                                     \
  default:                                                                     \
    logger_error("invalid operator %c", OP);                                   \
  };

#define _MUST_NOT_BE_NOISED(X, VIRTUAL_PRECISION)                              \
  /* if mode ieee, do not introduce noise */                                   \
  (BITMASKLIB_MODE == bitmask_mode_ieee) ||				\
  /* Check that we are not in a special case */				\
  (FPCLASSIFY(X) != FP_NORMAL && FPCLASSIFY(X) != FP_SUBNORMAL) ||	\
  /* In RR if the number is representable in current virtual precision, */ \
  /* do not add any noise if */						\
  (BITMASKLIB_MODE == bitmask_mode_ob && _IS_REPRESENTABLE(X, VIRTUAL_PRECISION))

#define _INEXACT(B)                                                            \
  do {                                                                         \
    const typeof(B.u) sign_size = GET_SIGN_SIZE(B.type);                       \
    const typeof(B.u) exp_size = GET_EXP_SIZE(B.type);                         \
    const typeof(B.u) pman_size = GET_PMAN_SIZE(B.type);                       \
    const typeof(B.u) mask_one = GET_MASK_ONE(B.type);                         \
    const int binary_t = GET_BINARYN_T(B.type);                                \
    typeof(B.u) bitmask = GET_BITMASK(B.type);                                 \
    if (FPCLASSIFY(*x) == FP_SUBNORMAL) {                                      \
      /* We must use the CLZ2 variant since bitfield type                      \
           are incompatible with _Generic feature */                           \
      const typeof(B.u) leading_0 =                                            \
          CLZ2(B.u, B.ieee.mantissa) - (sign_size + exp_size);                 \
      if (pman_size < (leading_0 + binary_t)) {                                \
        bitmask = mask_one;                                                    \
      } else {                                                                 \
        bitmask |= (mask_one << (pman_size - (leading_0 + binary_t)));         \
      }                                                                        \
    }                                                                          \
    if (BITMASKLIB_OPERATOR == bitmask_operator_rand) {                        \
      const typeof(B.u) rand_mask = GET_RANDOM_MASK(B.type);                   \
      B.ieee.mantissa ^= ~bitmask & rand_mask;                                 \
    } else if (BITMASKLIB_OPERATOR == bitmask_operator_one) {                  \
      B.u |= ~bitmask;                                                         \
    } else if (BITMASKLIB_OPERATOR == bitmask_operator_zero) {                 \
      B.u &= bitmask;                                                          \
    } else {                                                                   \
      __builtin_unreachable();                                                 \
    }                                                                          \
    *x = B.type;                                                               \
  } while (0);

static void _inexact_binary32(float *x) {
  if (_MUST_NOT_BE_NOISED(*x, BITMASKLIB_BINARY32_T)) {
    return;
  } else {
    binary32 b32 = {.f32 = *x};
    _INEXACT(b32)
  }
}

static void _inexact_binary64(double *x) {
  if (_MUST_NOT_BE_NOISED(*x, BITMASKLIB_BINARY64_T)) {
    return;
  } else {
    binary64 b64 = {.f64 = *x};
    _INEXACT(b64);
  }
}

#define _INEXACT_BINARYN(X)                                                    \
  _Generic(X, float * : _inexact_binary32, double * : _inexact_binary64)(X)

#define _BITMASK_BINARY_OP(A, B, OP, CTX)                                      \
  {                                                                            \
    typeof(A) RES = 0;                                                         \
    if (((t_context *)CTX)->daz) {                                             \
      A = DAZ(A);                                                              \
      B = DAZ(B);                                                              \
    }                                                                          \
    if (BITMASKLIB_MODE == bitmask_mode_ib ||                                  \
        BITMASKLIB_MODE == bitmask_mode_full) {                                \
      _INEXACT_BINARYN(&A);                                                    \
      _INEXACT_BINARYN(&B);                                                    \
    }                                                                          \
    PERFORM_BIN_OP(OP, RES, A, B);                                             \
    if (BITMASKLIB_MODE == bitmask_mode_ob ||                                  \
        BITMASKLIB_MODE == bitmask_mode_full) {                                \
      _INEXACT_BINARYN(&RES);                                                  \
    }                                                                          \
    if (((t_context *)CTX)->ftz) {                                             \
      RES = FTZ(RES);                                                          \
    }                                                                          \
    return RES;                                                                \
  }

static float _bitmask_binary32_binary_op(float a, float b,
                                         const bitmask_operations op,
                                         void *context) {
  _BITMASK_BINARY_OP(a, b, op, context)
}

static double _bitmask_binary64_binary_op(double a, double b,
                                          const bitmask_operations op,
                                          void *context) {
  _BITMASK_BINARY_OP(a, b, op, context)
}

/******************** BITMASK COMPARE FUNCTIONS ********************
 * Compare operations do not require BITMASK
 ****************************************************************/

/************************* FPHOOKS FUNCTIONS *************************
 * These functions correspond to those inserted into the source code
 * during source to source compilation and are replacement to floating
 * point operators
 **********************************************************************/

static void _interflop_add_float(float a, float b, float *c, void *context) {
  *c = _bitmask_binary32_binary_op(a, b, bitmask_add, context);
}

static void _interflop_sub_float(float a, float b, float *c, void *context) {
  *c = _bitmask_binary32_binary_op(a, b, bitmask_sub, context);
}

static void _interflop_mul_float(float a, float b, float *c, void *context) {
  *c = _bitmask_binary32_binary_op(a, b, bitmask_mul, context);
}

static void _interflop_div_float(float a, float b, float *c, void *context) {
  *c = _bitmask_binary32_binary_op(a, b, bitmask_div, context);
}

static void _interflop_add_double(double a, double b, double *c,
                                  void *context) {
  *c = _bitmask_binary64_binary_op(a, b, bitmask_add, context);
}

static void _interflop_sub_double(double a, double b, double *c,
                                  void *context) {
  *c = _bitmask_binary64_binary_op(a, b, bitmask_sub, context);
}

static void _interflop_mul_double(double a, double b, double *c,
                                  void *context) {
  *c = _bitmask_binary64_binary_op(a, b, bitmask_mul, context);
}

static void _interflop_div_double(double a, double b, double *c,
                                  void *context) {
  *c = _bitmask_binary64_binary_op(a, b, bitmask_div, context);
}

static struct argp_option options[] = {
    {key_prec_b32_str, KEY_PREC_B32, "PRECISION", 0,
     "select precision for binary32 (PRECISION > 0)", 0},
    {key_prec_b64_str, KEY_PREC_B64, "PRECISION", 0,
     "select precision for binary64 (PRECISION > 0)", 0},
    {key_mode_str, KEY_MODE, "MODE", 0,
     "select MCA mode among {ieee, mca, pb, rr}", 0},
    {key_operator_str, KEY_OPERATOR, "OPERATOR", 0,
     "select BITMASK operator among {zero, one, rand}", 0},
    {key_seed_str, KEY_SEED, "SEED", 0, "fix the random generator seed", 0},
    {key_daz_str, KEY_DAZ, 0, 0,
     "denormals-are-zero: sets denormals inputs to zero", 0},
    {key_ftz_str, KEY_FTZ, 0, 0, "flush-to-zero: sets denormal output to zero",
     0},
    {0}};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  t_context *ctx = (t_context *)state->input;
  char *endptr;
  switch (key) {
  case KEY_PREC_B32:
    /* precision for binary32 */
    errno = 0;
    int val = strtol(arg, &endptr, 10);
    if (errno != 0 || val <= 0) {
      logger_error("--%s invalid "
                   "value provided, must be a "
                   "positive integer.",
                   key_prec_b32_str);
    } else {
      _set_bitmask_precision_binary32(val);
    }
    break;
  case KEY_PREC_B64:
    /* precision for binary64 */
    errno = 0;
    val = strtol(arg, &endptr, 10);
    if (errno != 0 || val <= 0) {
      logger_error("--%s invalid "
                   "value provided, must be a "
                   "positive integer.",
                   key_prec_b64_str);
    } else {
      _set_bitmask_precision_binary64(val);
    }
    break;
  case KEY_MODE:
    /* mode */
    if (strcasecmp(BITMASK_MODE_STR[bitmask_mode_ieee], arg) == 0) {
      _set_bitmask_mode(bitmask_mode_ieee);
    } else if (strcasecmp(BITMASK_MODE_STR[bitmask_mode_full], arg) == 0) {
      _set_bitmask_mode(bitmask_mode_full);
    } else if (strcasecmp(BITMASK_MODE_STR[bitmask_mode_ib], arg) == 0) {
      _set_bitmask_mode(bitmask_mode_ib);
    } else if (strcasecmp(BITMASK_MODE_STR[bitmask_mode_ob], arg) == 0) {
      _set_bitmask_mode(bitmask_mode_ob);
    } else {
      logger_error("--%s invalid value provided, must be one of: "
                   "{ieee, full, ib, ob}.",
                   key_mode_str);
    }
    break;
  case KEY_OPERATOR:
    /* operator */
    if (strcasecmp(BITMASK_OPERATOR_STR[bitmask_operator_zero], arg) == 0) {
      _set_bitmask_operator(bitmask_operator_zero);
    } else if (strcasecmp(BITMASK_OPERATOR_STR[bitmask_operator_one], arg) ==
               0) {
      _set_bitmask_operator(bitmask_operator_one);
    } else if (strcasecmp(BITMASK_OPERATOR_STR[bitmask_operator_rand], arg) ==
               0) {
      _set_bitmask_operator(bitmask_operator_rand);
    } else {
      logger_error("--%s invalid value provided, must be "
                   "one of: "
                   "{zero, one, rand}.",
                   key_operator_str);
    }
    break;
  case KEY_SEED:
    /* set seed */
    errno = 0;
    ctx->choose_seed = true;
    ctx->seed = strtoull(arg, &endptr, 10);
    if (errno != 0) {
      logger_error("--%s invalid value provided, must be an "
                   "integer",
                   key_seed_str);
    }
    break;
  case KEY_DAZ:
    /* denormal-are-zero */
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

static struct argp argp = {options, parse_opt, "", "", NULL, NULL, NULL};

static void init_context(t_context *ctx) {
  ctx->choose_seed = false;
  ctx->seed = 0ULL;
  ctx->daz = false;
  ctx->ftz = false;
}

static void print_information_header(void *context) {
  t_context *ctx = (t_context *)context;

  logger_info(
      "load backend with "
      "%s = %d, "
      "%s = %d, "
      "%s = %s, "
      "%s = %s, "
      "%s = %s and "
      "%s = %s"
      "\n",
      key_prec_b32_str, BITMASKLIB_BINARY32_T, key_prec_b64_str,
      BITMASKLIB_BINARY64_T, key_mode_str, BITMASK_MODE_STR[BITMASKLIB_MODE],
      key_operator_str, BITMASK_OPERATOR_STR[BITMASKLIB_OPERATOR], key_daz_str,
      ctx->daz ? "true" : "false", key_ftz_str, ctx->ftz ? "true" : "false");
}

struct interflop_backend_interface_t interflop_init(int argc, char **argv,
                                                    void **context) {

  /* Initialize the logger */
  logger_init();

  _set_bitmask_precision_binary32(BITMASK_PRECISION_BINARY32_DEFAULT);
  _set_bitmask_precision_binary64(BITMASK_PRECISION_BINARY64_DEFAULT);
  _set_bitmask_mode(BITMASK_MODE_DEFAULT);
  _set_bitmask_operator(BITMASK_OPERATOR_DEFAULT);

  t_context *ctx = malloc(sizeof(t_context));
  *context = ctx;
  init_context(ctx);

  /* parse backend arguments */
  argp_parse(&argp, argc, argv, 0, 0, ctx);

  print_information_header(ctx);

  struct interflop_backend_interface_t interflop_backend_bitmask = {
      _interflop_add_float,
      _interflop_sub_float,
      _interflop_mul_float,
      _interflop_div_float,
      NULL,
      _interflop_add_double,
      _interflop_sub_double,
      _interflop_mul_double,
      _interflop_div_double,
      NULL};

  /* Initialize the seed */
  _set_bitmask_seed(ctx->choose_seed, ctx->seed);

  return interflop_backend_bitmask;
}
