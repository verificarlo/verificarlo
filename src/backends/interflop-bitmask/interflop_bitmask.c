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
#include "../../common/interflop.h"
#include "../../common/tinymt64.h"

typedef enum {
  KEY_PREC_B32,
  KEY_PREC_B64,
  KEY_MODE = 'm',
  KEY_OPERATOR = 'o',
  KEY_SEED = 's',
  KEY_DAZ,
  KEY_FTZ
} key_args;

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
static const char *BITMASKMODE[] = {"ieee", "full", "ib", "ob"};

/* define the available BITMASK */
typedef enum {
  bitmask_operator_zero,
  bitmask_operator_one,
  bitmask_operator_rand,
  _bitmask_operator_end_
} bitmask_operator;

/* string name of the bitmask */
static const char *BITMASKOPERATOR[] = {"zero", "one", "rand"};

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

/******************** BITMASK CONTROL FUNCTIONS *******************
 * The following functions are used to set virtual precision and
 * BITMASK mode of operation.
 ***************************************************************/

static void _set_bitmask_mode(bitmask_mode mode) {
  if (mode > _bitmask_mode_end_)
    errx(1, "interflop_bitmask: --mode invalid value provided, must be one of: "
            "{ieee, full, ib, ob}.");

  BITMASKLIB_MODE = mode;
}

static void _set_bitmask_operator(bitmask_operator bitmask) {
  if (bitmask > _bitmask_operator_end_)
    errx(
        1,
        "interflop_bitmask: --operator invalid value provided, must be one of: "
        "{zero, one, rand}.");

  BITMASKLIB_OPERATOR = bitmask;
}

static void _set_bitmask_precision_binary32(int precision) {
  if (precision < BITMASK_PRECISION_BINARY32_MIN) {
    errx(1,
         "interflop_bitmask: invalid precision for binary32 type. Must be "
         "greater than %d",
         BITMASK_PRECISION_BINARY32_MIN);
  } else if (precision > BITMASK_PRECISION_BINARY32_MAX) {
    warnx(
        "interflop_bitmask: precision for binary32 type is too high, no noise "
        "will be added");
  }
  BITMASKLIB_BINARY32_T = precision;
  binary32_bitmask = (BITMASKLIB_BINARY32_T <= FLOAT_PREC)
                         ? FLOAT_MASK_ONE
                               << (FLOAT_PREC - BITMASKLIB_BINARY32_T)
                         : FLOAT_MASK_ONE;
}

static void _set_bitmask_precision_binary64(int precision) {
  if (precision < BITMASK_PRECISION_BINARY64_MIN) {
    errx(1,
         "interflop_bitmask: invalid precision for binary64 type. Must be "
         "greater than %d",
         BITMASK_PRECISION_BINARY64_MIN);
  } else if (precision > BITMASK_PRECISION_BINARY64_MAX) {
    warnx(
        "interflop_bitmask: precision for binary64 type is too high, no noise "
        "will be added");
  }
  BITMASKLIB_BINARY64_T = precision;
  binary64_bitmask = (BITMASKLIB_BINARY64_T <= DOUBLE_PREC)
                         ? DOUBLE_MASK_ONE
                               << (DOUBLE_PREC - BITMASKLIB_BINARY64_T)
                         : DOUBLE_MASK_ONE;
}

/******************** BITMASK RANDOM FUNCTIONS ********************
 * The following functions are used to calculate the random bitmask
 ***************************************************************/

/* random generator internal state */
tinymt64_t random_state;

static uint64_t get_random_mask(void) {
  return tinymt64_generate_uint64(&random_state);
}

static uint64_t get_random_binary64_mask(void) {
  uint64_t mask = get_random_mask();
  return mask;
}

static uint32_t get_random_binary32_mask(void) {
  binary64 mask;
  mask.u64 = get_random_mask();
  return mask.u32[0];
}

static void _set_bitmask_seed(bool choose_seed, uint64_t seed) {
  if (choose_seed) {
    tinymt64_init(&random_state, seed);
  } else {
    const int key_length = 3;
    uint64_t init_key[key_length];
    struct timeval t1;
    gettimeofday(&t1, NULL);
    /* Hopefully the following seed is good enough for Montercarlo */
    init_key[0] = t1.tv_sec;
    init_key[1] = t1.tv_usec;
    init_key[2] = getpid();
    tinymt64_init_by_array(&random_state, init_key, key_length);
  }
}

/******************** BITMASK ARITHMETIC FUNCTIONS ********************
 * The following set of functions perform the BITMASK operation. Operands
 * They apply a bitmask to the result
 *******************************************************************/

/* perform_bin_op: applies the binary operator (op) to (a) and (b) */
/* and stores the result in (res) */
#define perform_bin_op(op, res, a, b)                                          \
  switch (op) {                                                                \
  case bitmask_add:                                                            \
    res = (a) + (b);                                                           \
    break;                                                                     \
  case bitmask_mul:                                                            \
    res = (a) * (b);                                                           \
    break;                                                                     \
  case bitmask_sub:                                                            \
    res = (a) - (b);                                                           \
    break;                                                                     \
  case bitmask_div:                                                            \
    res = (a) / (b);                                                           \
    break;                                                                     \
  default:                                                                     \
    perror("invalid operator in bitmask.\n");                                  \
    abort();                                                                   \
  };

static bool _is_representable_float(const float x) {
  binary32 b32 = {.f32 = x};
  if (b32.ieee.mantissa == 0) {
    return true;
  } else {
    uint32_t trailing_0 = __builtin_ctz(b32.ieee.mantissa);
    return FLOAT_PMAN_SIZE < (BITMASKLIB_BINARY32_T + trailing_0);
  }
}

static bool _is_representable_double(const double x) {
  binary64 b64 = {.f64 = x};
  if (b64.ieee.mantissa == 0) {
    return true;
  } else {
    uint64_t trailing_0 = __builtin_ctzl(b64.ieee.mantissa);
    return DOUBLE_PMAN_SIZE < (BITMASKLIB_BINARY64_T + trailing_0);
  }
}

static void _inexact_binary32(float *x) {

  if (BITMASKLIB_MODE == bitmask_mode_ieee) {
    return;
  } else if (fpclassify(*x) != FP_NORMAL && fpclassify(*x) != FP_SUBNORMAL) {
    return;
  } else if (BITMASKLIB_MODE == bitmask_mode_ob &&
             _is_representable_float(*x)) {
    return;
  } else {

    binary32 b32 = {.f32 = *x};

    uint32_t bitmask = binary32_bitmask;
    if (fpclassify(*x) == FP_SUBNORMAL) {
      const uint32_t leading_0 =
          __builtin_clz(b32.ieee.mantissa) - (FLOAT_SIGN_SIZE + FLOAT_EXP_SIZE);
      if (FLOAT_PMAN_SIZE < (leading_0 + BITMASKLIB_BINARY32_T)) {
        bitmask = FLOAT_MASK_ONE;
      } else {
        bitmask = bitmask |
                  (FLOAT_MASK_ONE
                   << (FLOAT_PMAN_SIZE - (leading_0 + BITMASKLIB_BINARY32_T)));
      }
    }

    if (BITMASKLIB_OPERATOR == bitmask_operator_rand) {
      uint32_t rand_binary32_mask = get_random_binary32_mask();
      b32.ieee.mantissa ^= ~bitmask & rand_binary32_mask;
    } else if (BITMASKLIB_OPERATOR == bitmask_operator_one) {
      b32.u32 |= ~binary32_bitmask;
    } else if (BITMASKLIB_OPERATOR == bitmask_operator_zero) {
      b32.u32 &= binary32_bitmask;
    } else {
      __builtin_unreachable();
    }

    *x = b32.f32;
  }
}

static void _inexact_binary64(double *x) {

  if (BITMASKLIB_MODE == bitmask_mode_ieee) {
    return;
  } else if (fpclassify(*x) != FP_NORMAL && fpclassify(*x) != FP_SUBNORMAL) {
    return;
  } else if (BITMASKLIB_MODE == bitmask_mode_ob &&
             _is_representable_double(*x)) {
    return;
  } else {

    binary64 b64 = {.f64 = *x};

    uint64_t bitmask = binary64_bitmask;
    if (fpclassify(*x) == FP_SUBNORMAL) {
      const uint64_t leading_0 = __builtin_clzl(b64.ieee.mantissa) -
                                 (DOUBLE_SIGN_SIZE + DOUBLE_EXP_SIZE);
      if (DOUBLE_PMAN_SIZE < (leading_0 + BITMASKLIB_BINARY64_T)) {
        bitmask = DOUBLE_MASK_ONE;
      } else {
        bitmask = bitmask |
                  (DOUBLE_MASK_ONE
                   << (DOUBLE_PMAN_SIZE - (leading_0 + BITMASKLIB_BINARY64_T)));
      }
    }

    if (BITMASKLIB_OPERATOR == bitmask_operator_rand) {
      const uint64_t rand_binary64_mask = get_random_binary64_mask();
      b64.ieee.mantissa ^= ~bitmask & rand_binary64_mask;
    } else if (BITMASKLIB_OPERATOR == bitmask_operator_one) {
      b64.u64 |= ~bitmask;
    } else if (BITMASKLIB_OPERATOR == bitmask_operator_zero) {
      b64.u64 &= bitmask;
    } else {
      __builtin_unreachable();
    }

    *x = b64.f64;
  }
}

static float _bitmask_binary32_binary_op(float a, float b,
                                         const bitmask_operations op,
                                         void *context) {
  float res = 0.0f;

  if (((t_context *)context)->daz) {
    a = (fpclassify(a) == FP_SUBNORMAL) ? 0.0f : a;
    b = (fpclassify(b) == FP_SUBNORMAL) ? 0.0f : b;
  }

  if (BITMASKLIB_MODE == bitmask_mode_ib ||
      BITMASKLIB_MODE == bitmask_mode_full) {
    _inexact_binary32(&a);
    _inexact_binary32(&b);
  }

  perform_bin_op(op, res, a, b);

  if (BITMASKLIB_MODE == bitmask_mode_ob ||
      BITMASKLIB_MODE == bitmask_mode_full) {
    _inexact_binary32(&res);
  }

  if (((t_context *)context)->ftz) {
    res = (fpclassify(res) == FP_SUBNORMAL) ? 0.0f : res;
  }

  return res;
}

static double _bitmask_binary64_binary_op(double a, double b,
                                          const bitmask_operations op,
                                          void *context) {
  double res = 0.0;

  if (((t_context *)context)->daz) {
    a = (fpclassify(a) == FP_SUBNORMAL) ? 0.0 : a;
    b = (fpclassify(b) == FP_SUBNORMAL) ? 0.0 : b;
  }

  if (BITMASKLIB_MODE == bitmask_mode_ib ||
      BITMASKLIB_MODE == bitmask_mode_full) {
    _inexact_binary64(&a);
    _inexact_binary64(&b);
  }

  perform_bin_op(op, res, a, b);

  if (BITMASKLIB_MODE == bitmask_mode_ob ||
      BITMASKLIB_MODE == bitmask_mode_full) {
    _inexact_binary64(&res);
  }

  if (((t_context *)context)->ftz) {
    res = (fpclassify(res) == FP_SUBNORMAL) ? 0.0 : res;
  }

  return res;
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
    /* --debug, sets the variable debug = true */
    {"precision-binary32", KEY_PREC_B32, "PRECISION", 0,
     "select precision for binary32 (PRECISION > 0)"},
    {"precision-binary64", KEY_PREC_B64, "PRECISION", 0,
     "select precision for binary64 (PRECISION > 0)"},
    {"mode", KEY_MODE, "MODE", 0,
     "select BITMASK mode among {ieee, full, ib, ob}"},
    {"operator", KEY_OPERATOR, "OPERATOR", 0,
     "select BITMASK operator among {zero, one, rand}"},
    {"seed", KEY_SEED, "SEED", 0, "fix the random generator seed"},
    {"daz", KEY_DAZ, 0, 0, "denormals-are-zero: sets denormals inputs to zero"},
    {"ftz", KEY_FTZ, 0, 0, "flush-to-zero: sets denormal output to zero"},
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
      errx(EXIT_FAILURE, "interflop_bitmask: --precision-binary32 invalid "
                         "value provided, must be a "
                         "positive integer.");
    } else {
      _set_bitmask_precision_binary32(val);
    }
    break;
  case KEY_PREC_B64:
    /* precision for binary64 */
    errno = 0;
    val = strtol(arg, &endptr, 10);
    if (errno != 0 || val <= 0) {
      errx(EXIT_FAILURE, "interflop_bitmask: --precision-binary64 invalid "
                         "value provided, must be a "
                         "positive integer.");
    } else {
      _set_bitmask_precision_binary64(val);
    }
    break;
  case KEY_MODE:
    /* mode */
    if (strcasecmp(BITMASKMODE[bitmask_mode_ieee], arg) == 0) {
      _set_bitmask_mode(bitmask_mode_ieee);
    } else if (strcasecmp(BITMASKMODE[bitmask_mode_full], arg) == 0) {
      _set_bitmask_mode(bitmask_mode_full);
    } else if (strcasecmp(BITMASKMODE[bitmask_mode_ib], arg) == 0) {
      _set_bitmask_mode(bitmask_mode_ib);
    } else if (strcasecmp(BITMASKMODE[bitmask_mode_ob], arg) == 0) {
      _set_bitmask_mode(bitmask_mode_ob);
    } else {
      errx(1,
           "interflop_bitmask: --mode invalid value provided, must be one of: "
           "{ieee, full, ib, ob}.");
    }
    break;
  case KEY_OPERATOR:
    /* mode */
    if (strcasecmp(BITMASKOPERATOR[bitmask_operator_zero], arg) == 0) {
      _set_bitmask_operator(bitmask_operator_zero);
    } else if (strcasecmp(BITMASKOPERATOR[bitmask_operator_one], arg) == 0) {
      _set_bitmask_operator(bitmask_operator_one);
    } else if (strcasecmp(BITMASKOPERATOR[bitmask_operator_rand], arg) == 0) {
      _set_bitmask_operator(bitmask_operator_rand);
    } else {
      errx(1, "interflop_bitmask: --operator invalid value provided, must be "
              "one of: "
              "{zero, one, rand}.");
    }
    break;
  case KEY_SEED:
    errno = 0;
    ctx->choose_seed = true;
    ctx->seed = strtoull(arg, &endptr, 10);
    if (errno != 0) {
      errx(1, "interflop_bitmask: --seed invalid value provided, must be an "
              "integer");
    }
    break;
  case KEY_DAZ:
    ctx->daz = true;
    break;
  case KEY_FTZ:
    ctx->ftz = true;
    break;
  default:
    return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

static struct argp argp = {options, parse_opt, "", ""};

static void init_context(t_context *ctx) {
  ctx->choose_seed = false;
  ctx->seed = 0ULL;
  ctx->daz = false;
  ctx->ftz = false;
}

struct interflop_backend_interface_t interflop_init(int argc, char **argv,
                                                    void **context) {

  _set_bitmask_precision_binary32(BITMASK_PRECISION_BINARY32_DEFAULT);
  _set_bitmask_precision_binary64(BITMASK_PRECISION_BINARY64_DEFAULT);
  _set_bitmask_mode(BITMASK_MODE_DEFAULT);
  _set_bitmask_operator(BITMASK_OPERATOR_DEFAULT);

  t_context *ctx = malloc(sizeof(t_context));
  *context = ctx;
  init_context(ctx);

  /* parse backend arguments */
  argp_parse(&argp, argc, argv, 0, 0, ctx);

  warnx("interflop_bitmask: loaded backend with precision-binary32 = %d, "
        "precision-binary64 = %d, mode = %s and operator = %s",
        BITMASKLIB_BINARY32_T, BITMASKLIB_BINARY64_T,
        BITMASKMODE[BITMASKLIB_MODE], BITMASKOPERATOR[BITMASKLIB_OPERATOR]);

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
