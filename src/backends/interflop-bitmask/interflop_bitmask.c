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
// Changelog:
//
// 2021-10-13 Switched random number generator from TinyMT64 to the one
// provided by the libc. The backend is now re-entrant. Pthread and OpenMP
// threads are now supported.
// Generation of hook functions is now done through macros, shared accross
// backends.

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
#include "../../common/rng/vfc_rng.h"

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

/* global thread identifier */
static pid_t global_tid = 0;

/* helper data structure to centralize the data used for random number
 * generation */
static __thread rng_state_t rng_state;

static uint64_t get_random_mask(void) {
  return get_rand_uint64(&rng_state, &global_tid);
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

#define _INEXACT(CTX, B)                                                       \
  do {                                                                         \
    const typeof(B.u) sign_size = GET_SIGN_SIZE(B.type);                       \
    const typeof(B.u) exp_size = GET_EXP_SIZE(B.type);                         \
    const typeof(B.u) pman_size = GET_PMAN_SIZE(B.type);                       \
    const typeof(B.u) mask_one = GET_MASK_ONE(B.type);                         \
    const int binary_t = GET_BINARYN_T(B.type);                                \
    typeof(B.u) bitmask = GET_BITMASK(B.type);                                 \
    _init_rng_state_struct(&rng_state, ((t_context *)CTX)->choose_seed,        \
                           (unsigned long long)(((t_context *)CTX)->seed),     \
                           false);                                             \
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

static void _inexact_binary32(void *context, float *x) {
  if (_MUST_NOT_BE_NOISED(*x, BITMASKLIB_BINARY32_T)) {
    return;
  } else {
    binary32 b32 = {.f32 = *x};
    _INEXACT(context, b32)
  }
}

static void _inexact_binary64(void *context, double *x) {
  if (_MUST_NOT_BE_NOISED(*x, BITMASKLIB_BINARY64_T)) {
    return;
  } else {
    binary64 b64 = {.f64 = *x};
    _INEXACT(context, b64);
  }
}

#define _INEXACT_BINARYN(CTX, X)                                               \
  _Generic(X, float * : _inexact_binary32, double * : _inexact_binary64)(CTX, X)

#define _BITMASK_BINARY_OP(A, B, OP, CTX)                                      \
  {                                                                            \
    typeof(A) RES = 0;                                                         \
    if (((t_context *)CTX)->daz) {                                             \
      A = DAZ(A);                                                              \
      B = DAZ(B);                                                              \
    }                                                                          \
    if (BITMASKLIB_MODE == bitmask_mode_ib ||                                  \
        BITMASKLIB_MODE == bitmask_mode_full) {                                \
      _INEXACT_BINARYN(CTX, &A);                                               \
      _INEXACT_BINARYN(CTX, &B);                                               \
    }                                                                          \
    PERFORM_BIN_OP(OP, RES, A, B);                                             \
    if (BITMASKLIB_MODE == bitmask_mode_ob ||                                  \
        BITMASKLIB_MODE == bitmask_mode_full) {                                \
      _INEXACT_BINARYN(CTX, &RES);                                             \
    }                                                                          \
    if (((t_context *)CTX)->ftz) {                                             \
      RES = FTZ(RES);                                                          \
    }                                                                          \
    return RES;                                                                \
  }

static float _bitmask_binary32_binary_op(float a, float b,
                                         const bitmask_operations op,
                                         void *context) {
  _BITMASK_BINARY_OP(a, b, op, context);
}

static double _bitmask_binary64_binary_op(double a, double b,
                                          const bitmask_operations op,
                                          void *context) {
  _BITMASK_BINARY_OP(a, b, op, context);
}

/******************** BITMASK COMPARE FUNCTIONS ********************
 * Compare operations do not require BITMASK
 ****************************************************************/

/************************* FPHOOKS FUNCTIONS *************************
 * These functions correspond to those inserted into the source code
 * during source to source compilation and are replacement to floating
 * point operators
 **********************************************************************/

_INTERFLOP_OP_CALL(float, add, bitmask_add, _bitmask_binary32_binary_op);

_INTERFLOP_OP_CALL(float, sub, bitmask_sub, _bitmask_binary32_binary_op);

_INTERFLOP_OP_CALL(float, mul, bitmask_mul, _bitmask_binary32_binary_op);

_INTERFLOP_OP_CALL(float, div, bitmask_div, _bitmask_binary32_binary_op);

_INTERFLOP_OP_CALL(double, add, bitmask_add, _bitmask_binary64_binary_op);

_INTERFLOP_OP_CALL(double, sub, bitmask_sub, _bitmask_binary64_binary_op);

_INTERFLOP_OP_CALL(double, mul, bitmask_mul, _bitmask_binary64_binary_op);

_INTERFLOP_OP_CALL(double, div, bitmask_div, _bitmask_binary64_binary_op);

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
      NULL,
      NULL,
      NULL,
      NULL,
      NULL};

  /* The seed for the RNG is initialized upon the first request for a random
     number */

  _init_rng_state_struct(&rng_state, ctx->choose_seed, ctx->seed, false);

  return interflop_backend_bitmask;
}
