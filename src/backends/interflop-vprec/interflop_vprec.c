/*****************************************************************************
 *                                                                           *
 *  This file is part of Verificarlo.                                        *
 *                                                                           *
 *  Copyright (c) 2015                                                       *
 *     Universite de Versailles St-Quentin-en-Yvelines                       *
 *     CMLA, Ecole Normale Superieure de Cachan                              *
 *  Copyright (c) 2019-2020                                                  *
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
// 2018-07-7 Initial version from scratch
//
// 2019-11-25 Code refactoring, format conversions moved to
// ../../common/vprec_tools.c
//

#include <argp.h>
#include <err.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include "../../common/float_const.h"
#include "../../common/float_struct.h"
#include "../../common/float_utils.h"
#include "../../common/interflop.h"
#include "../../common/logger.h"
#include "../../common/vprec_tools.h"

typedef enum {
  KEY_PREC_B32,
  KEY_PREC_B64,
  KEY_RANGE_B32,
  KEY_RANGE_B64,
  KEY_MODE = 'm',
  KEY_DAZ = 'd',
  KEY_FTZ = 'f'
} key_args;

static const char key_prec_b32_str[] = "precision-binary32";
static const char key_prec_b64_str[] = "precision-binary64";
static const char key_range_b32_str[] = "range-binary32";
static const char key_range_b64_str[] = "range-binary64";
static const char key_mode_str[] = "mode";
static const char key_daz_str[] = "daz";
static const char key_ftz_str[] = "ftz";

typedef struct {
  bool daz;
  bool ftz;
} t_context;

/* define the available VPREC modes of operation */
typedef enum {
  vprecmode_ieee,
  vprecmode_full,
  vprecmode_ib,
  vprecmode_ob,
  _vprecmode_end_
} vprec_mode;

/* Modes' names */
static const char *VPREC_MODE_STR[] = {"ieee", "full", "ib", "ob"};

/* define the possible VPREC operation */
typedef enum {
  vprec_add = '+',
  vprec_sub = '-',
  vprec_mul = '*',
  vprec_div = '/',
} vprec_operation;

/* define default environment variables and default parameters */

/* default values of precision and range for binary32 */
#define VPREC_PRECISION_BINARY32_MIN 1
#define VPREC_PRECISION_BINARY32_MAX FLOAT_PMAN_SIZE
#define VPREC_PRECISION_BINARY32_DEFAULT FLOAT_PMAN_SIZE
#define VPREC_RANGE_BINARY32_MIN 1
#define VPREC_RANGE_BINARY32_MAX FLOAT_EXP_SIZE
#define VPREC_RANGE_BINARY32_DEFAULT FLOAT_EXP_SIZE

/* default values of precision and range for binary64 */
#define VPREC_PRECISION_BINARY64_MIN 1
#define VPREC_PRECISION_BINARY64_MAX DOUBLE_PMAN_SIZE
#define VPREC_PRECISION_BINARY64_DEFAULT DOUBLE_PMAN_SIZE
#define VPREC_RANGE_BINARY64_MIN 1
#define VPREC_RANGE_BINARY64_MAX DOUBLE_EXP_SIZE
#define VPREC_RANGE_BINARY64_DEFAULT DOUBLE_EXP_SIZE

/* common default values */
#define VPREC_MODE_DEFAULT vprecmode_ob

/* variables that control precision, range and mode */
static vprec_mode VPRECLIB_MODE = VPREC_MODE_DEFAULT;
static int VPRECLIB_BINARY32_PRECISION = VPREC_PRECISION_BINARY32_DEFAULT;
static int VPRECLIB_BINARY64_PRECISION = VPREC_PRECISION_BINARY64_DEFAULT;
static int VPRECLIB_BINARY32_RANGE = VPREC_RANGE_BINARY32_DEFAULT;
static int VPRECLIB_BINARY64_RANGE = VPREC_RANGE_BINARY64_DEFAULT;

static float _vprec_binary32_binary_op(float a, float b,
                                       const vprec_operation op, void *context);
static double _vprec_binary64_binary_op(double a, double b,
                                        const vprec_operation op,
                                        void *context);

/******************** VPREC CONTROL FUNCTIONS *******************
 * The following functions are used to set virtual precision and
 * VPREC mode of operation.
 ***************************************************************/

void _set_vprec_mode(vprec_mode mode) {
  if (mode >= _vprecmode_end_) {
    logger_error("invalid mode provided, must be one of: "
                 "{ieee, full, ib, ob}.");
  } else {
    VPRECLIB_MODE = mode;
  }
}

void _set_vprec_precision_binary32(int precision) {
  if (precision < VPREC_PRECISION_BINARY32_MIN) {
    logger_error("invalid precision provided for binary32."
                 "Must be greater than %d",
                 VPREC_PRECISION_BINARY32_MIN);
  } else if (VPREC_PRECISION_BINARY32_MAX < precision) {
    logger_error("invalid precision provided, "
                 "must be lower than (%d)",
                 VPREC_RANGE_BINARY32_MAX);
  } else {
    VPRECLIB_BINARY32_PRECISION = precision;
  }
}

void _set_vprec_range_binary32(int range) {
  if (range < VPREC_RANGE_BINARY32_MIN) {
    logger_error("invalid range provided for binary32."
                 "Must be greater than %d",
                 VPREC_RANGE_BINARY32_MIN);
  } else if (VPREC_RANGE_BINARY32_MAX < range) {
    logger_error("invalid range provided, "
                 "must be lower than (%d)",
                 VPREC_RANGE_BINARY32_MAX);
  } else {
    VPRECLIB_BINARY32_RANGE = range;
  }
}

void _set_vprec_precision_binary64(int precision) {
  if (precision < VPREC_PRECISION_BINARY64_MIN) {
    logger_error("invalid precision provided for binary64."
                 "Must be greater than %d",
                 VPREC_PRECISION_BINARY64_MIN);
  } else if (VPREC_PRECISION_BINARY64_MAX < precision) {
    logger_error("invalid precision provided, "
                 "must be lower than (%d)",
                 VPREC_RANGE_BINARY64_MAX);
  } else {
    VPRECLIB_BINARY64_PRECISION = precision;
  }
}

void _set_vprec_range_binary64(int range) {
  if (range < VPREC_RANGE_BINARY64_MIN) {
    logger_error("invalid range provided for binary64."
                 "Must be greater than %d",
                 VPREC_RANGE_BINARY64_MIN);
  } else if (VPREC_RANGE_BINARY64_MAX < range) {
    logger_error("invalid range provided, "
                 "must be lower than (%d)",
                 VPREC_RANGE_BINARY64_MAX);
  } else {
    VPRECLIB_BINARY64_RANGE = range;
  }
}

/******************** VPREC ARITHMETIC FUNCTIONS ********************
 * The following set of functions perform the VPREC operation. Operands
 * are first correctly rounded to the target precison format if inbound
 * is set, the operation is then perform using IEEE hw and
 * correct rounding to the target precision format is done if outbound
 * is set.
 *******************************************************************/

/* perform_bin_op: applies the binary operator (op) to (a) and (b) */
/* and stores the result in (res) */
#define perform_binary_op(op, res, a, b)                                       \
  switch (op) {                                                                \
  case vprec_add:                                                              \
    res = (a) + (b);                                                           \
    break;                                                                     \
  case vprec_mul:                                                              \
    res = (a) * (b);                                                           \
    break;                                                                     \
  case vprec_sub:                                                              \
    res = (a) - (b);                                                           \
    break;                                                                     \
  case vprec_div:                                                              \
    res = (a) / (b);                                                           \
    break;                                                                     \
  default:                                                                     \
    logger_error("invalid operator %c", op);                                   \
  };

static inline float _vprec_binary32_binary_op(float a, float b,
                                              const vprec_operation op,
                                              void *context) {
  float res = 0;

  /* test if a or b are special cases */
  if (!isfinite(a) || !isfinite(b)) {
    perform_binary_op(op, res, a, b);
    return res;
  }

  /* round to zero or set to infinity if underflow or overflow compare to
   * VPRECLIB_BINARY32_RANGE */
  int emax = (1 << (VPRECLIB_BINARY32_RANGE - 1)) - 1;
  int emin = (emax > 1) ? 1 - emax : -1;

  /* if IB or FULL, bound operand precision */
  if ((VPRECLIB_MODE == vprecmode_full) || (VPRECLIB_MODE == vprecmode_ib)) {

    /* get operand exponent */
    binary32 aexp = {.f32 = a};
    binary32 bexp = {.f32 = b};

    aexp.s32 = ((FLOAT_GET_EXP & aexp.u32) >> FLOAT_PMAN_SIZE) - FLOAT_EXP_COMP;
    bexp.s32 = ((FLOAT_GET_EXP & bexp.u32) >> FLOAT_PMAN_SIZE) - FLOAT_EXP_COMP;

    /* check for overflow or underflow in target range */
    bool sp_case = false;
    if (aexp.s32 > emax) {
      a = a * INFINITY;
      sp_case = true;
    }

    if (bexp.s32 > emax) {
      b = b * INFINITY;
      sp_case = true;
    }

    /* if exp of a or b between emin and emin -target prec  (mantissa lenth) */
    /* denormal number case require a rounding and truncation to the
     * representable part */
    /* if below emin-target prec */
    /* set to correctly signed zero */
    if (aexp.s32 <= emin) {
      if (((t_context *)context)->daz) {
        a = 0;
      } else {
        a = handle_binary32_denormal(a, emin, aexp.u32,
                                     VPRECLIB_BINARY32_PRECISION);
      }
    }

    if (bexp.s32 <= emin) {
      if (((t_context *)context)->daz) {
        b = 0;
      } else {
        b = handle_binary32_denormal(b, emin, bexp.u32,
                                     VPRECLIB_BINARY32_PRECISION);
      }
    }

    /* Specials ops must be placed after denormal handling  */
    /* If one of the operand raises an underflow the operation */
    /* has a different behavior. Example: x*Inf != 0*Inf */
    if (sp_case) {
      perform_binary_op(op, res, a, b);
      return res;
    }

    /* else normal case, can be executed even if a or b
       previously rounded and truncated as denormal */
    if (VPRECLIB_BINARY32_PRECISION < FLOAT_PMAN_SIZE) {
      a = round_binary32_normal(a, VPRECLIB_BINARY32_PRECISION);
      b = round_binary32_normal(b, VPRECLIB_BINARY32_PRECISION);
    }
  };

  /* perform standard floating point operation on the correctly rounded
   * arguments */
  perform_binary_op(op, res, a, b);

  if (!isfinite(res)) {
    return res;
  }

  /* if OB or FULL mode, bound results precision */
  if ((VPRECLIB_MODE == vprecmode_full) || (VPRECLIB_MODE == vprecmode_ob)) {
    binary32 resexp = {.f32 = res};
    resexp.s32 =
        ((FLOAT_GET_EXP & resexp.u32) >> FLOAT_PMAN_SIZE) - FLOAT_EXP_COMP;

    /* check for overflow in target range */
    if (resexp.s32 > emax) {
      resexp.f32 = res;
      return res * INFINITY;
    }

    /* handle underflow or denormal for res */
    if (resexp.s32 <= emin) {
      if (((t_context *)context)->ftz) {
        res = 0;
      } else {
        res = handle_binary32_denormal(res, emin, resexp.u32,
                                       VPRECLIB_BINARY32_PRECISION);
      }
    }

    /* normal case for res */
    if (VPRECLIB_BINARY32_PRECISION < FLOAT_PMAN_SIZE) {
      res = round_binary32_normal(res, VPRECLIB_BINARY32_PRECISION);
    }
  }
  return res;
}

static inline double _vprec_binary64_binary_op(double a, double b,
                                               const vprec_operation op,
                                               void *context) {
  double res = 0;

  /* test if a or b are special cases */
  if (!isfinite(a) || !isfinite(b)) {
    perform_binary_op(op, res, a, b);
    return res;
  }

  /* round to zero or set to infinity if underflow or overflow compare to
   * VPRECLIB_BINARY64_RANGE */
  int emax = (1 << (VPRECLIB_BINARY64_RANGE - 1)) - 1;
  int emin = (emax > 1) ? 1 - emax : -1;

  /* if IB or FULL, bound operand precision */
  if ((VPRECLIB_MODE == vprecmode_full) || (VPRECLIB_MODE == vprecmode_ib)) {

    /* get operand exponent */
    binary64 aexp = {.f64 = a};
    binary64 bexp = {.f64 = b};

    aexp.s64 =
        ((DOUBLE_GET_EXP & aexp.u64) >> DOUBLE_PMAN_SIZE) - DOUBLE_EXP_COMP;
    bexp.s64 =
        ((DOUBLE_GET_EXP & bexp.u64) >> DOUBLE_PMAN_SIZE) - DOUBLE_EXP_COMP;

    /* check for overflow or underflow in target range */
    bool sp_case = false;
    if (aexp.s64 > emax) {
      a = a * INFINITY;
      sp_case = true;
    }

    if (bexp.s64 > emax) {
      b = b * INFINITY;
      sp_case = true;
    }

    /* if exp of a or b between emin and emin -target prec  (mantissa lenth) */
    /* denormal number case require a roundin and truncation to the
     * representable part */
    /* if below emin-target prec */
    /* set to correctly signed zero */
    if (aexp.s64 <= emin) {
      if (((t_context *)context)->daz) {
        a = 0;
      } else {
        a = handle_binary64_denormal(a, emin, aexp.u64,
                                     VPRECLIB_BINARY64_PRECISION);
      }
    }

    if (bexp.s64 <= emin) {
      if (((t_context *)context)->daz) {
        b = 0;
      } else {
        b = handle_binary64_denormal(b, emin, bexp.u64,
                                     VPRECLIB_BINARY64_PRECISION);
      }
    }

    /* Specials ops must be placed after denormal handling  */
    /* If one of the operand raises an underflow the operation */
    /* has a different behavior. Example: x*Inf != 0*Inf */
    if (sp_case) {
      perform_binary_op(op, res, a, b);
      return res;
    }

    /* else normal case, can be executed even if a or b previously rounded and
     * truncated as denormal */
    if (VPRECLIB_BINARY64_PRECISION < DOUBLE_PMAN_SIZE) {
      a = round_binary64_normal(a, VPRECLIB_BINARY64_PRECISION);
      b = round_binary64_normal(b, VPRECLIB_BINARY64_PRECISION);
    }
  }

  /* perform standard floating point operation on the correctly rounded
   * arguments */
  perform_binary_op(op, res, a, b);

  if (!isfinite(res)) {
    return res;
  }

  /* if OB or FULL mode, bound results precision */
  if ((VPRECLIB_MODE == vprecmode_full) || (VPRECLIB_MODE == vprecmode_ob)) {
    binary64 resexp = {.f64 = res};
    resexp.s64 =
        ((DOUBLE_GET_EXP & resexp.u64) >> DOUBLE_PMAN_SIZE) - DOUBLE_EXP_COMP;

    /* check for overflow in target range */
    if (resexp.s64 > emax) {
      return res * INFINITY;
    }

    /* handle underflow or denormal for res */
    if (resexp.s64 <= emin) {
      if (((t_context *)context)->ftz) {
        res = 0;
      } else {
        res = handle_binary64_denormal(res, emin, resexp.u64,
                                       VPRECLIB_BINARY64_PRECISION);
      }
    }

    /* normal case for res */
    if (VPRECLIB_BINARY64_PRECISION < DOUBLE_PMAN_SIZE) {
      res = round_binary64_normal(res, VPRECLIB_BINARY64_PRECISION);
    }
  };

  return res;
}

/************************* FPHOOKS FUNCTIONS *************************
 * These functions correspond to those inserted into the source code
 * during source to source compilation and are replacement to floating
 * point operators
 **********************************************************************/

static void _interflop_add_float(float a, float b, float *c, void *context) {
  *c = _vprec_binary32_binary_op(a, b, vprec_add, context);
}

static void _interflop_sub_float(float a, float b, float *c, void *context) {
  *c = _vprec_binary32_binary_op(a, b, vprec_sub, context);
}

static void _interflop_mul_float(float a, float b, float *c, void *context) {
  *c = _vprec_binary32_binary_op(a, b, vprec_mul, context);
}

static void _interflop_div_float(float a, float b, float *c, void *context) {
  *c = _vprec_binary32_binary_op(a, b, vprec_div, context);
}

static void _interflop_add_double(double a, double b, double *c,
                                  void *context) {
  *c = _vprec_binary64_binary_op(a, b, vprec_add, context);
}

static void _interflop_sub_double(double a, double b, double *c,
                                  void *context) {
  *c = _vprec_binary64_binary_op(a, b, vprec_sub, context);
}

static void _interflop_mul_double(double a, double b, double *c,
                                  void *context) {
  *c = _vprec_binary64_binary_op(a, b, vprec_mul, context);
}

static void _interflop_div_double(double a, double b, double *c,
                                  void *context) {
  *c = _vprec_binary64_binary_op(a, b, vprec_div, context);
}

static struct argp_option options[] = {
    /* --debug, sets the variable debug = true */
    {key_prec_b32_str, KEY_PREC_B32, "PRECISION", 0,
     "select precision for binary32 (PRECISION >= 0)"},
    {key_prec_b64_str, KEY_PREC_B64, "PRECISION", 0,
     "select precision for binary64 (PRECISION >= 0)"},
    {key_range_b32_str, KEY_RANGE_B32, "RANGE", 0,
     "select range for binary32 (0 < RANGE && RANGE <= 8)"},
    {key_range_b64_str, KEY_RANGE_B64, "RANGE", 0,
     "select range for binary64 (0 < RANGE && RANGE <= 11)"},
    {key_mode_str, KEY_MODE, "MODE", 0,
     "select VPREC mode among {ieee, full, ib, ob}"},
    {key_daz_str, KEY_DAZ, 0, 0,
     "denormals-are-zero: sets denormals inputs to zero"},
    {key_ftz_str, KEY_FTZ, 0, 0, "flush-to-zero: sets denormal output to zero"},
    {0}};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  t_context *ctx = (t_context *)state->input;
  char *endptr;
  int val = -1;
  switch (key) {
  case KEY_PREC_B32:
    /* precision */
    errno = 0;
    val = strtol(arg, &endptr, 10);
    if (errno != 0 || val < VPREC_PRECISION_BINARY32_MIN) {
      logger_error("--%s invalid value provided, must be a "
                   "positive integer.",
                   key_prec_b32_str);
    } else if (val > VPREC_PRECISION_BINARY32_MAX) {
      logger_error("--%s invalid value provided, "
                   "must lower than IEEE binary32 precision (%d)",
                   key_prec_b32_str, VPREC_PRECISION_BINARY32_MAX);
    } else {
      _set_vprec_precision_binary32(val);
    }
    break;
  case KEY_PREC_B64:
    /* precision */
    errno = 0;
    val = strtol(arg, &endptr, 10);
    if (errno != 0 || val < VPREC_PRECISION_BINARY64_MIN) {
      logger_error("--%s invalid value provided, must be a "
                   "positive integer.",
                   key_prec_b64_str);
    } else if (val > VPREC_PRECISION_BINARY64_MAX) {
      logger_error("--%s invalid value provided, "
                   "must be lower than IEEE binary64 precision (%d)",
                   key_prec_b64_str, VPREC_PRECISION_BINARY64_MAX);
    } else {
      _set_vprec_precision_binary64(val);
    }
    break;
  case KEY_RANGE_B32:
    /* precision */
    errno = 0;
    val = strtol(arg, &endptr, 10);
    if (errno != 0 || val < VPREC_RANGE_BINARY32_MIN) {
      logger_error("--%s invalid value provided, must be a "
                   "positive integer.",
                   key_range_b32_str);
    } else if (val > VPREC_RANGE_BINARY32_MAX) {
      logger_error("--%s invalid value provided, "
                   "must be lower than IEEE binary32 range size (%d)",
                   key_range_b32_str, VPREC_RANGE_BINARY32_MAX);
    } else {
      _set_vprec_range_binary32(val);
    }
    break;
  case KEY_RANGE_B64:
    /* precision */
    errno = 0;
    val = strtol(arg, &endptr, 10);
    if (errno != 0 || val < VPREC_RANGE_BINARY64_MIN) {
      logger_error("--%s invalid value provided, must be a "
                   "positive integer.",
                   key_range_b64_str);
    } else if (val > VPREC_RANGE_BINARY64_MAX) {
      logger_error("--%s invalid value provided, "
                   "must be lower than IEEE binary64 range size (%d)",
                   key_range_b64_str, VPREC_RANGE_BINARY64_MAX);
    } else {
      _set_vprec_range_binary64(val);
    }
    break;
  case KEY_MODE:
    /* mode */
    if (strcasecmp(VPREC_MODE_STR[vprecmode_ieee], arg) == 0) {
      _set_vprec_mode(vprecmode_ieee);
    } else if (strcasecmp(VPREC_MODE_STR[vprecmode_full], arg) == 0) {
      _set_vprec_mode(vprecmode_full);
    } else if (strcasecmp(VPREC_MODE_STR[vprecmode_ib], arg) == 0) {
      _set_vprec_mode(vprecmode_ib);
    } else if (strcasecmp(VPREC_MODE_STR[vprecmode_ob], arg) == 0) {
      _set_vprec_mode(vprecmode_ob);
    } else {
      logger_error("--%s invalid value provided, must be one of: "
                   "{ieee, full, ib, ob}.",
                   key_mode_str);
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

static struct argp argp = {options, parse_opt, "", ""};

void init_context(t_context *ctx) {
  ctx->daz = false;
  ctx->ftz = false;
}

void print_information_header(void *context) {
  /* Environnement variable to disable loading message */
  char *silent_load_env = getenv("VFC_BACKENDS_SILENT_LOAD");
  bool silent_load =
      ((silent_load_env == NULL) || (strcasecmp(silent_load_env, "True") != 0))
          ? false
          : true;

  if (silent_load)
    return;

  t_context *ctx = (t_context *)context;

  logger_info(
      "load backend with "
      "%s = %d, "
      "%s = %d, "
      "%s = %d, "
      "%s = %d, "
      "%s = %s, "
      "%s = %s and "
      "%s = %s"
      "\n",
      key_prec_b32_str, VPRECLIB_BINARY32_PRECISION, key_range_b32_str,
      VPRECLIB_BINARY32_RANGE, key_prec_b64_str, VPRECLIB_BINARY64_PRECISION,
      key_range_b64_str, VPRECLIB_BINARY64_RANGE, key_mode_str,
      VPREC_MODE_STR[VPRECLIB_MODE], key_daz_str, ctx->daz ? "true" : "false",
      key_ftz_str, ctx->ftz ? "true" : "false");
}

struct interflop_backend_interface_t interflop_init(int argc, char **argv,
                                                    void **context) {

  /* Initialize the logger */
  logger_init();

  /* Setting to default values */
  _set_vprec_precision_binary32(VPREC_PRECISION_BINARY32_DEFAULT);
  _set_vprec_precision_binary64(VPREC_PRECISION_BINARY64_DEFAULT);
  _set_vprec_range_binary32(VPREC_RANGE_BINARY32_DEFAULT);
  _set_vprec_range_binary64(VPREC_RANGE_BINARY64_DEFAULT);
  _set_vprec_mode(VPREC_MODE_DEFAULT);

  t_context *ctx = malloc(sizeof(t_context));
  *context = ctx;
  init_context(ctx);

  /* parse backend arguments */
  argp_parse(&argp, argc, argv, 0, 0, ctx);

  print_information_header(ctx);

  struct interflop_backend_interface_t interflop_backend_vprec = {
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

  return interflop_backend_vprec;
}
