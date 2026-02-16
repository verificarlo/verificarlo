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
 *  Copyright (c) 2019-2026                                                  *\
 *     Verificarlo Contributors                                              *\
 *                                                                           *\
 ****************************************************************************/
// Changelog:
//
// 2018-07-7 Initial version from scratch
//
// 2019-11-25 Code refactoring, format conversions moved to
// ../../common/vprec_tools.c
//

#include <argp.h>
#include <math.h>

#include "common/vprec_tools.h"
#include "interflop/common/float_const.h"
#include "interflop/common/float_struct.h"
#include "interflop/common/float_utils.h"
#include "interflop/fma/interflop_fma.h"
#include "interflop/interflop.h"
#include "interflop/interflop_stdlib.h"
#include "interflop/iostream/logger.h"
#include "interflop_vprec.h"
#include "interflop_vprec_function_instrumentation.h"

static const char backend_name[] = "interflop-vprec";
static const char backend_version[] = "1.x-dev";

static const char key_prec_b32_str[] = "precision-binary32";
static const char key_prec_b64_str[] = "precision-binary64";
static const char key_range_b32_str[] = "range-binary32";
static const char key_range_b64_str[] = "range-binary64";
static const char key_preset_str[] = "preset";
static const char key_mode_str[] = "mode";
static const char key_err_mode_str[] = "error-mode";
static const char key_err_exp_str[] = "max-abs-error-exponent";
static const char key_daz_str[] = "daz";
static const char key_ftz_str[] = "ftz";

/* variables that control precision, range and mode */

/* Modes' names */
static const char *const VPREC_MODE_STR[] = {[vprecmode_ieee] = "ieee",
                                             [vprecmode_full] = "full",
                                             [vprecmode_ib] = "ib",
                                             [vprecmode_ob] = "ob"};

static const char *const VPREC_ERR_MODE_STR[] = {[vprec_err_mode_rel] = "rel",
                                                 [vprec_err_mode_abs] = "abs",
                                                 [vprec_err_mode_all] = "all"};

static const char *const VPREC_PRESET_STR[] = {
    [vprec_preset_binary16] = "binary16",
    [vprec_preset_binary32] = "binary32",
    [vprec_preset_bfloat16] = "bfloat16",
    [vprec_preset_tensorfloat] = "tensorfloat",
    [vprec_preset_fp24] = "fp24",
    [vprec_preset_PXR24] = "PXR24"};

static float _vprec_binary32_binary_op(float a, float b, vprec_operation op,
                                       void *context);
static double _vprec_binary64_binary_op(double a, double b, vprec_operation op,
                                        void *context);

/******************** VPREC CONTROL FUNCTIONS *******************
 * The following functions are used to set virtual precision,
 * VPREC mode of operation and instrumentation mode.
 ***************************************************************/

void _set_vprec_mode(vprec_mode mode, vprec_context_t *ctx) {
  if (mode >= _vprecmode_end_) {
    logger_error("invalid mode provided, must be one of: "
                 "{ieee, full, ib, ob}.");
  } else {
    ctx->mode = mode;
  }
}

void _set_vprec_precision_binary32(int precision, vprec_context_t *ctx) {
  if (precision < VPREC_PRECISION_BINARY32_MIN) {
    logger_error("invalid precision provided for binary32. "
                 "Must be greater than %d",
                 VPREC_PRECISION_BINARY32_MIN);
  } else if (VPREC_PRECISION_BINARY32_MAX < precision) {
    logger_error("invalid precision provided for binary32. "
                 "Must be lower than %d",
                 VPREC_PRECISION_BINARY32_MAX);
  } else {
    ctx->binary32_precision = precision;
  }
}

void _set_vprec_range_binary32(int range, vprec_context_t *ctx) {
  if (range < VPREC_RANGE_BINARY32_MIN) {
    logger_error("invalid range provided for binary32. "
                 "Must be greater than %d",
                 VPREC_RANGE_BINARY32_MIN);
  } else if (VPREC_RANGE_BINARY32_MAX < range) {
    logger_error("invalid range provided for binary32. "
                 "Must be lower than %d",
                 VPREC_RANGE_BINARY32_MAX);
  } else {
    ctx->binary32_range = range;
  }
}

void _set_vprec_precision_binary64(int precision, vprec_context_t *ctx) {
  if (precision < VPREC_PRECISION_BINARY64_MIN) {
    logger_error("invalid precision provided for binary64 (%d). "
                 "Must be greater than %d",
                 precision, VPREC_PRECISION_BINARY64_MIN);
  } else if (VPREC_PRECISION_BINARY64_MAX < precision) {
    logger_error("invalid precision provided for binary64. "
                 "Must be lower than %d",
                 VPREC_PRECISION_BINARY64_MAX);
  } else {
    ctx->binary64_precision = precision;
  }
}

void _set_vprec_range_binary64(int range, vprec_context_t *ctx) {
  if (range < VPREC_RANGE_BINARY64_MIN) {
    logger_error("invalid range provided for binary64. "
                 "Must be greater than %d",
                 VPREC_RANGE_BINARY64_MIN);
  } else if (VPREC_RANGE_BINARY64_MAX < range) {
    logger_error("invalid range provided for binary64. "
                 "Must be lower than %d",
                 VPREC_RANGE_BINARY64_MAX);
  } else {
    ctx->binary64_range = range;
  }
}

void _set_vprec_error_mode(vprec_err_mode mode, vprec_context_t *ctx) {
  if (mode >= _vprec_err_mode_end_) {
    logger_error("invalid error mode provided, must be one of: "
                 "{rel, abs, all}.");
  } else {
    switch (mode) {
    case vprec_err_mode_rel:
      ctx->relErr = true;
      ctx->absErr = false;
      break;
    case vprec_err_mode_abs:
      ctx->relErr = false;
      ctx->absErr = true;
      break;
    case vprec_err_mode_all:
      ctx->relErr = true;
      ctx->absErr = true;
    default:
      break;
    }
  }
}

void _set_vprec_max_abs_err_exp(long exponent, vprec_context_t *ctx) {
  ctx->absErr_exp = (int)exponent;
}

const char *_get_error_mode_str(vprec_context_t *ctx) {
  if (ctx->relErr && ctx->absErr) {
    return VPREC_ERR_MODE_STR[vprec_err_mode_all];
  }
  if (ctx->relErr && !ctx->absErr) {
    return VPREC_ERR_MODE_STR[vprec_err_mode_rel];
  }
  if (!ctx->relErr && ctx->absErr) {
    return VPREC_ERR_MODE_STR[vprec_err_mode_abs];
  }
  return NULL;
}

static vprec_preset_precision _get_vprec_preset_precision(vprec_preset preset) {
  switch (preset) {
  case vprec_preset_binary16:
    return vprec_preset_precision_binary16;
  case vprec_preset_binary32:
    return vprec_preset_precision_binary32;
  case vprec_preset_bfloat16:
    return vprec_preset_precision_bfloat16;
  case vprec_preset_tensorfloat:
    return vprec_preset_precision_tensorfloat;
  case vprec_preset_fp24:
    return vprec_preset_precision_fp24;
  case vprec_preset_PXR24:
    return vprec_preset_precision_PXR24;
  default:
    logger_error("invalid preset provided, must be one of: "
                 "{binary16, binary32, binary64, bfloat16, tensorfloat, "
                 "fp24, PXR24}");
    return _vprec_preset_precision_end_;
  }
}

static vprec_preset_range _get_vprec_preset_range(vprec_preset preset) {
  switch (preset) {
  case vprec_preset_binary16:
    return vprec_preset_range_binary16;
  case vprec_preset_binary32:
    return vprec_preset_range_binary32;
  case vprec_preset_bfloat16:
    return vprec_preset_range_bfloat16;
  case vprec_preset_tensorfloat:
    return vprec_preset_range_tensorfloat;
  case vprec_preset_fp24:
    return vprec_preset_range_fp24;
  case vprec_preset_PXR24:
    return vprec_preset_range_PXR24;
  default:
    logger_error("invalid preset provided, must be one of: "
                 "{binary16, binary32, binary64, bfloat16, tensorfloat, "
                 "fp24, PXR24}");
    return _vprec_preset_range_end_;
  }
}

const char *get_vprec_mode_name(vprec_mode mode) {
  if (mode >= _vprecmode_end_) {
    return NULL;
  }
  return VPREC_MODE_STR[mode];
}

void _set_vprec_daz(bool daz, vprec_context_t *ctx) { ctx->daz = daz; }

void _set_vprec_ftz(bool ftz, vprec_context_t *ctx) { ctx->ftz = ftz; }

/******************** VPREC HELPER FUNCTIONS *******************
 * The following functions are used to set virtual precision,
 * VPREC mode of operation and instrumentation mode.
 ***************************************************************/
extern int compute_absErr_vprec_binary32(bool isDenormal,
                                         vprec_context_t *currentContext,
                                         int expDiff, int binary32_precision);
inline int compute_absErr_vprec_binary32(bool isDenormal,
                                         vprec_context_t *currentContext,
                                         int expDiff, int binary32_precision) {
  /* this function is used only when in vprec error mode abs and all,
   * so there is no need to handle vprec error mode rel */
  if (isDenormal == true) {
    /* denormal, or underflow case */
    if (currentContext->relErr == true) {
      /* vprec error mode all */
      if (abs(currentContext->absErr_exp) < binary32_precision) {
        return currentContext->absErr_exp;
      }
      return binary32_precision;
    } /* vprec error mode abs */
    return currentContext->absErr_exp;
  } /* normal case */
  if (currentContext->relErr == true) {
    /* vprec error mode all */
    if (expDiff < binary32_precision) {
      return expDiff;
    }
    return binary32_precision;
  } /* vprec error mode abs */
  if (expDiff < FLOAT_PMAN_SIZE) {
    return expDiff;
  }
  return FLOAT_PMAN_SIZE;
}
extern int compute_absErr_vprec_binary64(bool isDenormal,
                                         vprec_context_t *currentContext,
                                         int expDiff, int binary64_precision);
inline int compute_absErr_vprec_binary64(bool isDenormal,
                                         vprec_context_t *currentContext,
                                         int expDiff, int binary64_precision) {
  /* this function is used only when in vprec error mode abs and all,
   * so there is no need to handle vprec error mode rel */
  if (isDenormal == true) {
    /* denormal, or underflow case */
    if (currentContext->relErr == true) {
      /* vprec error mode all */
      if (abs(currentContext->absErr_exp) < binary64_precision) {
        return currentContext->absErr_exp;
      }
      return binary64_precision;
    } /* vprec error mode abs */
    return currentContext->absErr_exp;

  } /* normal case */
  if (currentContext->relErr == true) {
    /* vprec error mode all */
    if (expDiff < binary64_precision) {
      return expDiff;
    }
    return binary64_precision;

  } /* vprec error mode abs */
  if (expDiff < DOUBLE_PMAN_SIZE) {
    return expDiff;
  }
  return DOUBLE_PMAN_SIZE;
}

extern float handle_binary32_normal_absErr(float a, int32_t aexp,
                                           int binary32_precision,
                                           vprec_context_t *currentContext);
inline float handle_binary32_normal_absErr(float a, int32_t aexp,
                                           int binary32_precision,
                                           vprec_context_t *currentContext) {
  /* absolute error mode, or both absolute and relative error modes */
  int expDiff = aexp - currentContext->absErr_exp;
  float retVal = NAN;

  if (expDiff < -1) {
    /* equivalent to underflow on the precision given by absolute error */
    retVal = 0;
  } else if (expDiff == -1) {
    /* case when the number is just below the absolute error threshold,
      but will round to one ulp on the format given by the absolute error;
      this needs to be handled separately, as round_binary32_normal cannot
      generate this number */
    retVal = copysignf(fpow2i(currentContext->absErr_exp), a);
  } else {
    /* normal case for the absolute error mode */
    int binary32_precision_adjusted = compute_absErr_vprec_binary32(
        false, currentContext, expDiff, binary32_precision);
    retVal = round_binary32_normal(a, binary32_precision_adjusted);
  }

  return retVal;
}

extern double handle_binary64_normal_absErr(double a, int64_t aexp,
                                            int binary64_precision,
                                            vprec_context_t *currentContext);
inline double handle_binary64_normal_absErr(double a, int64_t aexp,
                                            int binary64_precision,
                                            vprec_context_t *currentContext) {
  /* absolute error mode, or both absolute and relative error modes */
  int64_t expDiff = aexp - currentContext->absErr_exp;
  double retVal = NAN;

  if (expDiff < -1) {
    /* equivalent to underflow on the precision given by absolute error */
    retVal = 0;
  } else if (expDiff == -1) {
    /* case when the number is just below the absolute error threshold,
      but will round to one ulp on the format given by the absolute error;
      this needs to be handled separately, as round_binary32_normal cannot
      generate this number */
    retVal = copysign(pow2i(currentContext->absErr_exp), a);
  } else {
    /* normal case for the absolute error mode */
    int binary64_precision_adjusted = compute_absErr_vprec_binary64(
        false, currentContext, (int)expDiff, binary64_precision);
    retVal = round_binary64_normal(a, binary64_precision_adjusted);
  }

  return retVal;
}

/******************** VPREC ARITHMETIC FUNCTIONS ********************
 * The following set of functions perform the VPREC operation. Operands
 * are first correctly rounded to the target precison format if inbound
 * is set, the operation is then perform using IEEE hw and
 * correct rounding to the target precision format is done if outbound
 * is set.
 *******************************************************************/

#define PERFORM_FMA(A, B, C)                                                   \
  _Generic(A,                                                                  \
      float: interflop_fma_binary32,                                           \
      double: interflop_fma_binary64,                                          \
      _Float128: interflop_fma_binary128)(A, B, C)

/* perform_binary_op: applies the binary operator (op) to (a) and (b) */
/* and stores the result in (res) */
#define perform_binary_op(op, res, a, b)                                       \
  switch (op) {                                                                \
  case vprec_add:                                                              \
    (res) = (a) + (b);                                                         \
    break;                                                                     \
  case vprec_mul:                                                              \
    (res) = (a) * (b);                                                         \
    break;                                                                     \
  case vprec_sub:                                                              \
    (res) = (a) - (b);                                                         \
    break;                                                                     \
  case vprec_div:                                                              \
    (res) = (a) / (b);                                                         \
    break;                                                                     \
  default:                                                                     \
    logger_error("invalid operator %c", op);                                   \
  };

/* perform_ternary_op: applies the ternary operator (op) to (a), (b) and (c) */
/* and stores the result in (res) */
#define perform_ternary_op(op, res, a, b, c)                                   \
  switch (op) {                                                                \
  case vprec_fma:                                                              \
    (res) = PERFORM_FMA((a), (b), (c));                                        \
    break;                                                                     \
  default:                                                                     \
    logger_error("invalid operator %c", op);                                   \
  };

// Round the float with the given precision
float _vprec_round_binary32(float a, char is_input, void *context,
                            int binary32_range, int binary32_precision) {
  vprec_context_t *currentContext = (vprec_context_t *)context;
  /* test if 'a' is a special case */
  if (!isfinite(a)) {
    return a;
  }

  /* round to zero or set to infinity if underflow or overflow compared to
   * ctx->binary32_range */
  int emax = (1 << (binary32_range - 1)) - 1;
  /* here emin is the smallest exponent in the *normal* range */
  int emin = 1 - emax;

  binary32 aexp = {.f32 = a};
  aexp.s32 = ((FLOAT_GET_EXP & aexp.u32) >> FLOAT_PMAN_SIZE) - FLOAT_EXP_COMP;

  /* check for overflow in target range */
  if (aexp.s32 >= emax) {
    if (aexp.s32 == emax) {
      /* For values close to MAX_FLOAT, overflow should be checked after
       * rounding. */
      float b = round_binary32_normal(a, binary32_precision);
      binary32 bexp = {.f32 = b};
      bexp.s32 = (int32_t)((FLOAT_GET_EXP & bexp.u32) >> FLOAT_PMAN_SIZE) -
                 FLOAT_EXP_COMP;
      if (bexp.s32 > emax) {
        a = a * INFINITY;
        return a;
      }
    } else {
      a = a * INFINITY;
      return a;
    }
  }

  /* check for underflow in target range */
  if (aexp.s32 < emin) {
    /* underflow case: possibly a denormal */
    if ((currentContext->daz && is_input) ||
        (currentContext->ftz && !is_input)) {
      return a * 0; // preserve sign
    }
    if (FP_ZERO == fpclassify(a)) {
      return a;
    }
    if (currentContext->absErr == true) {
      /* absolute error mode, or both absolute and relative error modes */
      int binary32_precision_adjusted = compute_absErr_vprec_binary32(
          true, currentContext, 0, binary32_precision);
      a = handle_binary32_denormal(a, emin, binary32_precision_adjusted);
    } else {
      /* relative error mode */
      a = handle_binary32_denormal(a, emin, binary32_precision);
    }

  } else {
    /* else, normal case: can be executed even if a
     previously rounded and truncated as denormal */
    if (currentContext->absErr == true) {
      /* absolute error mode, or both absolute and relative error modes */
      a = handle_binary32_normal_absErr(a, aexp.s32, binary32_precision,
                                        currentContext);
    } else {
      /* relative error mode */
      a = round_binary32_normal(a, binary32_precision);
    }
  }

  return a;
}

// Round the double with the given precision
double _vprec_round_binary64(double a, char is_input, void *context,
                             int binary64_range, int binary64_precision) {
  vprec_context_t *currentContext = (vprec_context_t *)context;

  /* test if 'a' is a special case */
  if (!isfinite(a)) {
    return a;
  }

  /* round to zero or set to infinity if underflow or overflow compare to
   * ctx->binary64_range */
  int emax = (1 << (binary64_range - 1)) - 1;
  /* here emin is the smallest exponent in the *normal* range */
  int emin = 1 - emax;

  binary64 aexp = {.f64 = a};
  aexp.s64 = (int64_t)((DOUBLE_GET_EXP & aexp.u64) >> DOUBLE_PMAN_SIZE) -
             DOUBLE_EXP_COMP;

  /* check for overflow in target range */
  if (aexp.s64 >= emax) {
    if (aexp.s64 == emax) {
      /* For values close to MAX_FLOAT, overflow should be checked after
       * rounding. */
      double b = round_binary64_normal(a, binary64_precision);
      binary64 bexp = {.f64 = b};
      bexp.s64 = (int64_t)((DOUBLE_GET_EXP & bexp.u64) >> DOUBLE_PMAN_SIZE) -
                 DOUBLE_EXP_COMP;
      if (bexp.s64 > emax) {
        a = a * INFINITY;
        return a;
      }
    } else {
      a = a * INFINITY;
      return a;
    }
  }

  /* check for underflow in target range */
  if (aexp.s64 < emin) {
    /* underflow case: possibly a denormal */
    if ((currentContext->daz && is_input) ||
        (currentContext->ftz && !is_input)) {
      return a * 0; // preserve sign
    }
    if (FP_ZERO == fpclassify(a)) {
      return a;
    }
    if (currentContext->absErr == true) {
      /* absolute error mode, or both absolute and relative error modes */
      int binary64_precision_adjusted = compute_absErr_vprec_binary64(
          true, currentContext, 0, binary64_precision);
      a = handle_binary64_denormal(a, emin, binary64_precision_adjusted);
    } else {
      /* relative error mode */
      a = handle_binary64_denormal(a, emin, binary64_precision);
    }

  } else {
    /* else, normal case: can be executed even if a
     previously rounded and truncated as denormal */
    if (currentContext->absErr == true) {
      /* absolute error mode, or both absolute and relative error modes */
      a = handle_binary64_normal_absErr(a, aexp.s64, binary64_precision,
                                        currentContext);
    } else {
      /* relative error mode */
      a = round_binary64_normal(a, binary64_precision);
    }
  }

  return a;
}

static inline float _vprec_binary32_binary_op(float a, float b,
                                              const vprec_operation op,
                                              void *context) {
  vprec_context_t *ctx = (vprec_context_t *)context;
  float res = 0;

  logger_debug("[Inputs] binary32: a=%+.6a b=%+.6a op=%c\n", a, b, op);

  if ((ctx->mode == vprecmode_full) || (ctx->mode == vprecmode_ib)) {
    a = _vprec_round_binary32(a, 1, context, ctx->binary32_range,
                              ctx->binary32_precision);
    b = _vprec_round_binary32(b, 1, context, ctx->binary32_range,
                              ctx->binary32_precision);
    logger_debug("[Round ] binary32: a=%+.6a b=%+.6a op=%c\n", a, b, op);
  }

  perform_binary_op(op, res, a, b);
  logger_debug("[Result] binary32: res=%.6a\n", res);

  if ((ctx->mode == vprecmode_full) || (ctx->mode == vprecmode_ob)) {
    res = _vprec_round_binary32(res, 0, context, ctx->binary32_range,
                                ctx->binary32_precision);
    logger_debug("[Round ] binary32: res=%+.6a\n", res);
  }

  return res;
}

static inline double _vprec_binary64_binary_op(double a, double b,
                                               const vprec_operation op,
                                               void *context) {
  vprec_context_t *ctx = (vprec_context_t *)context;
  double res = 0;
  logger_debug("[Inputs] binary64: a=%+.13a b=%+.13a op=%c\n", a, b, op);

  if ((ctx->mode == vprecmode_full) || (ctx->mode == vprecmode_ib)) {
    a = _vprec_round_binary64(a, 1, context, ctx->binary64_range,
                              ctx->binary64_precision);
    b = _vprec_round_binary64(b, 1, context, ctx->binary64_range,
                              ctx->binary64_precision);
    logger_debug("[Round ] binary64: a=%+.13a b=%+.13a op=%c\n", a, b, op);
  }

  perform_binary_op(op, res, a, b);
  logger_debug("[Result] binary64: res=%.13a\n", res);

  if ((ctx->mode == vprecmode_full) || (ctx->mode == vprecmode_ob)) {
    res = _vprec_round_binary64(res, 0, context, ctx->binary64_range,
                                ctx->binary64_precision);
    logger_debug("[Round ] binary64: res=%+.13a\n", res);
  }

  return res;
}

static inline float _vprec_binary32_ternary_op(float a, float b, float c,
                                               const vprec_operation op,
                                               void *context) {

  vprec_context_t *ctx = (vprec_context_t *)context;
  float res = 0;
  if (ctx->mode == vprecmode_ib || ctx->mode == vprecmode_full) {
    a = _vprec_round_binary32(a, 1, context, ctx->binary32_range,
                              ctx->binary32_precision);
    b = _vprec_round_binary32(b, 1, context, ctx->binary32_range,
                              ctx->binary32_precision);
    c = _vprec_round_binary32(c, 1, context, ctx->binary32_range,
                              ctx->binary32_precision);
  }

  perform_ternary_op(op, res, a, b, c);

  if (ctx->mode == vprecmode_ob || ctx->mode == vprecmode_full) {
    res = _vprec_round_binary32(res, 0, context, ctx->binary32_range,
                                ctx->binary32_precision);
  }

  return res;
}

static inline double _vprec_binary64_ternary_op(double a, double b, double c,
                                                const vprec_operation op,
                                                void *context) {
  vprec_context_t *ctx = (vprec_context_t *)context;
  double res = 0;
  if (ctx->mode == vprecmode_ib || ctx->mode == vprecmode_full) {
    a = _vprec_round_binary64(a, 1, context, ctx->binary64_range,
                              ctx->binary64_precision);
    b = _vprec_round_binary64(b, 1, context, ctx->binary64_range,
                              ctx->binary64_precision);
    c = _vprec_round_binary64(c, 1, context, ctx->binary64_range,
                              ctx->binary64_precision);
  }

  perform_ternary_op(op, res, a, b, c);

  if (ctx->mode == vprecmode_ob || ctx->mode == vprecmode_full) {
    res = _vprec_round_binary64(res, 0, context, ctx->binary64_range,
                                ctx->binary64_precision);
  }

  return res;
}

// Set precision for internal operations and round input arguments for a given
// function call
void INTERFLOP_VPREC_API(enter_function)(interflop_function_stack_t *stack,
                                         void *context, int nb_args,
                                         va_list ap) {
  _vfi_enter_function(stack, context, nb_args, ap);
}

// Set precision for internal operations and round output arguments for a given
// function call
void INTERFLOP_VPREC_API(exit_function)(interflop_function_stack_t *stack,
                                        void *context, int nb_args,
                                        va_list ap) {
  _vfi_exit_function(stack, context, nb_args, ap);
}

/************************* FPHOOKS FUNCTIONS *************************
 * These functions correspond to those inserted into the source code
 * during source to source compilation and are replacement to floating
 * point operators
 **********************************************************************/

void INTERFLOP_VPREC_API(add_float)(float a, float b, float *c, void *context) {
  *c = _vprec_binary32_binary_op(a, b, vprec_add, context);
}

void INTERFLOP_VPREC_API(sub_float)(float a, float b, float *c, void *context) {
  *c = _vprec_binary32_binary_op(a, b, vprec_sub, context);
}

void INTERFLOP_VPREC_API(mul_float)(float a, float b, float *c, void *context) {
  *c = _vprec_binary32_binary_op(a, b, vprec_mul, context);
}

void INTERFLOP_VPREC_API(div_float)(float a, float b, float *c, void *context) {
  *c = _vprec_binary32_binary_op(a, b, vprec_div, context);
}

void INTERFLOP_VPREC_API(add_double)(double a, double b, double *c,
                                     void *context) {
  *c = _vprec_binary64_binary_op(a, b, vprec_add, context);
}

void INTERFLOP_VPREC_API(sub_double)(double a, double b, double *c,
                                     void *context) {
  *c = _vprec_binary64_binary_op(a, b, vprec_sub, context);
}

void INTERFLOP_VPREC_API(mul_double)(double a, double b, double *c,
                                     void *context) {
  *c = _vprec_binary64_binary_op(a, b, vprec_mul, context);
}

void INTERFLOP_VPREC_API(div_double)(double a, double b, double *c,
                                     void *context) {
  *c = _vprec_binary64_binary_op(a, b, vprec_div, context);
}
#define MACROMIN(a, b) ((a) < (b) ? (a) : (b))

void INTERFLOP_VPREC_API(cast_double_to_float)(double a, float *b,
                                               void *context) {
  vprec_context_t *ctx = (vprec_context_t *)context;
  if ((ctx->mode == vprecmode_ieee)) {
    *b = (float)a;
    return;
  }

  if ((ctx->mode == vprecmode_ob)) {
    *b = (float)_vprec_round_binary64(a, 0, context, ctx->binary32_range,
                                      ctx->binary32_precision);
    return;
  }

  if ((ctx->mode == vprecmode_full)) {
    // double rounding is avoided
    // daz is ignored (switch O to 1 does not solve the problem: denormal
    // depends on ctx->binary64_*) hypothesis  ctx->binary32_* < ctx->binary64_*
    *b = (float)_vprec_round_binary64(a, 0, context, ctx->binary32_range,
                                      ctx->binary32_precision);
    return;
  }

  if ((ctx->mode == vprecmode_ib)) {
    // double rounding is avoided thanks to MACROMIN and float constant
    *b = (float)_vprec_round_binary64(
        a, 1, context, MACROMIN(ctx->binary64_range, VPREC_RANGE_BINARY32_MAX),
        MACROMIN(ctx->binary64_precision, VPREC_PRECISION_BINARY32_MAX));
    return;
  }
}

void INTERFLOP_VPREC_API(fma_float)(float a, float b, float c, float *res,
                                    void *context) {
  *res = _vprec_binary32_ternary_op(a, b, c, vprec_fma, context);
}

void INTERFLOP_VPREC_API(fma_double)(double a, double b, double c, double *res,
                                     void *context) {
  *res = _vprec_binary64_ternary_op(a, b, c, vprec_fma, context);
}

void INTERFLOP_VPREC_API(user_call)(void *context, interflop_call_id id,
                                    va_list ap) {
  vprec_context_t *ctx = (vprec_context_t *)context;
  switch (id) {
  case INTERFLOP_SET_PRECISION_BINARY32:
    _set_vprec_precision_binary32(va_arg(ap, int), ctx);
    break;
  case INTERFLOP_SET_PRECISION_BINARY64:
    _set_vprec_precision_binary64(va_arg(ap, int), ctx);
    break;
  case INTERFLOP_SET_RANGE_BINARY32:
    _set_vprec_range_binary32(va_arg(ap, int), ctx);
    break;
  case INTERFLOP_SET_RANGE_BINARY64:
    _set_vprec_range_binary64(va_arg(ap, int), ctx);
    break;
  default:
    logger_warning("Unknown interflop_call id (=%d)", id);
    break;
  }
}

void INTERFLOP_VPREC_API(finalize)(void *context) {
  vprec_context_t *ctx = (vprec_context_t *)context;
  _vfi_finalize(ctx);
}

const char *INTERFLOP_VPREC_API(get_backend_name)(void) { return backend_name; }

const char *INTERFLOP_VPREC_API(get_backend_version)(void) {
  return backend_version;
}

void _vprec_check_stdlib() {
  INTERFLOP_CHECK_IMPL(calloc);
  INTERFLOP_CHECK_IMPL(exit);
  INTERFLOP_CHECK_IMPL(fclose);
  INTERFLOP_CHECK_IMPL(fgets);
  INTERFLOP_CHECK_IMPL(fopen);
  INTERFLOP_CHECK_IMPL(fprintf);
  INTERFLOP_CHECK_IMPL(free);
  INTERFLOP_CHECK_IMPL(getenv);
  INTERFLOP_CHECK_IMPL(gettid);
  INTERFLOP_CHECK_IMPL(malloc);
  INTERFLOP_CHECK_IMPL(sprintf);
  INTERFLOP_CHECK_IMPL(strcasecmp);
  INTERFLOP_CHECK_IMPL(strcmp);
  INTERFLOP_CHECK_IMPL(strcpy);
  INTERFLOP_CHECK_IMPL(strncpy);
  INTERFLOP_CHECK_IMPL(strerror);
  INTERFLOP_CHECK_IMPL(strtok_r);
  INTERFLOP_CHECK_IMPL(strtol);
  INTERFLOP_CHECK_IMPL(vfprintf);
  INTERFLOP_CHECK_IMPL(vwarnx);
}

/* allocate the context */
void _vprec_alloc_context(void **context) {
  vprec_context_t *ctx =
      (vprec_context_t *)interflop_malloc(sizeof(vprec_context_t));
  _vfi_alloc_context(ctx);
  *context = ctx;
}

/* intialize the context */
static void _vprec_init_context(vprec_context_t *ctx) {
  ctx->binary32_precision = VPREC_PRECISION_BINARY32_DEFAULT;
  ctx->binary32_range = VPREC_RANGE_BINARY32_DEFAULT;
  ctx->binary64_precision = VPREC_PRECISION_BINARY64_DEFAULT;
  ctx->binary64_range = VPREC_RANGE_BINARY64_DEFAULT;
  ctx->mode = VPREC_MODE_DEFAULT;
  ctx->relErr = true;
  ctx->absErr = false;
  ctx->absErr_exp = -DOUBLE_EXP_MIN;
  ctx->daz = false;
  ctx->ftz = false;
  _vfi_init_context(ctx);
}

void INTERFLOP_VPREC_API(pre_init)(interflop_panic_t panic, File *stream,
                                   void **context) {
  interflop_set_handler("panic", panic);
  _vprec_check_stdlib();

  /* Initialize the logger */
  logger_init(panic, stream, backend_name);

  /* allocate the context */
  _vprec_alloc_context(context);
  _vprec_init_context((vprec_context_t *)*context);
}

static const struct argp_option options[] = {
    /* --debug, sets the variable debug = true */
    {key_prec_b32_str, KEY_PREC_B32, "PRECISION", 0,
     "select precision for binary32 (PRECISION >= 0)", 0},
    {key_prec_b64_str, KEY_PREC_B64, "PRECISION", 0,
     "select precision for binary64 (PRECISION >= 0)", 0},
    {key_range_b32_str, KEY_RANGE_B32, "RANGE", 0,
     "select range for binary32 (0 < RANGE && RANGE <= 8)", 0},
    {key_range_b64_str, KEY_RANGE_B64, "RANGE", 0,
     "select range for binary64 (0 < RANGE && RANGE <= 11)", 0},
    {key_preset_str, KEY_PRESET, "PRESET", 0,
     "select a default PRESET setting among {binary16, binary32, binary64, "
     "bfloat16, tensorfloat, fp24, PXR24}\n"
     "Format (range, precision) : "
     "binary16 (5, 10), binary32 (8, 23), "
     "bfloat16 (8, 7), tensorfloat (8, 10), "
     "fp24 (7, 16), PXR24 (8, 15)",
     0},
    {key_mode_str, KEY_MODE, "MODE", 0,
     "select VPREC mode among {ieee, full, ib, ob}", 0},
    {key_err_mode_str, KEY_ERR_MODE, "ERROR_MODE", 0,
     "select error mode among {rel, abs, all}", 0},
    {key_err_exp_str, KEY_ERR_EXP, "MAX_ABS_ERROR_EXPONENT", 0,
     "select magnitude of the maximum absolute error", 0},
    {key_daz_str, KEY_DAZ, 0, 0,
     "denormals-are-zero: sets denormals inputs to zero", 0},
    {key_ftz_str, KEY_FTZ, 0, 0, "flush-to-zero: sets denormal output to zero",
     0},
    {0}};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  vprec_context_t *ctx = (vprec_context_t *)state->input;
  state->child_inputs[0] = ctx;
  char *endptr = NULL;
  int val = -1;
  int precision = 0;
  int range = 0;
  int error = 0;

  switch (key) {
  case KEY_PREC_B32:
    /* precision */
    error = 0;
    val = (int)interflop_strtol(arg, &endptr, &error);
    if (error != 0 || val < VPREC_PRECISION_BINARY32_MIN) {
      logger_error("--%s invalid value provided, must be a "
                   "positive integer.",
                   key_prec_b32_str);
    } else if (val > VPREC_PRECISION_BINARY32_MAX) {
      logger_error("--%s invalid value provided, "
                   "must lower than IEEE binary32 precision (%d)",
                   key_prec_b32_str, VPREC_PRECISION_BINARY32_MAX);
    } else {
      _set_vprec_precision_binary32(val, ctx);
    }
    break;
  case KEY_PREC_B64:
    /* precision */
    error = 0;
    val = (int)interflop_strtol(arg, &endptr, &error);
    if (error != 0 || val < VPREC_PRECISION_BINARY64_MIN) {
      logger_error("--%s invalid value provided, must be a "
                   "positive integer.",
                   key_prec_b64_str);
    } else if (val > VPREC_PRECISION_BINARY64_MAX) {
      logger_error("--%s invalid value provided, "
                   "must be lower than IEEE binary64 precision (%d)",
                   key_prec_b64_str, VPREC_PRECISION_BINARY64_MAX);
    } else {
      _set_vprec_precision_binary64(val, ctx);
    }
    break;
  case KEY_RANGE_B32:
    /* precision */
    error = 0;
    val = (int)interflop_strtol(arg, &endptr, &error);
    if (error != 0 || val < VPREC_RANGE_BINARY32_MIN) {
      logger_error("--%s invalid value provided, must be a "
                   "positive integer.",
                   key_range_b32_str);
    } else if (val > VPREC_RANGE_BINARY32_MAX) {
      logger_error("--%s invalid value provided, "
                   "must be lower than IEEE binary32 range size (%d)",
                   key_range_b32_str, VPREC_RANGE_BINARY32_MAX);
    } else {
      _set_vprec_range_binary32(val, ctx);
    }
    break;
  case KEY_RANGE_B64:
    /* precision */
    error = 0;
    val = (int)interflop_strtol(arg, &endptr, &error);
    if (error != 0 || val < VPREC_RANGE_BINARY64_MIN) {
      logger_error("--%s invalid value provided, must be a "
                   "positive integer.",
                   key_range_b64_str);
    } else if (val > VPREC_RANGE_BINARY64_MAX) {
      logger_error("--%s invalid value provided, "
                   "must be lower than IEEE binary64 range size (%d)",
                   key_range_b64_str, VPREC_RANGE_BINARY64_MAX);
    } else {
      _set_vprec_range_binary64(val, ctx);
    }
    break;
  case KEY_MODE:
    /* mode */
    if (interflop_strcasecmp(VPREC_MODE_STR[vprecmode_ieee], arg) == 0) {
      _set_vprec_mode(vprecmode_ieee, ctx);
    } else if (interflop_strcasecmp(VPREC_MODE_STR[vprecmode_full], arg) == 0) {
      _set_vprec_mode(vprecmode_full, ctx);
    } else if (interflop_strcasecmp(VPREC_MODE_STR[vprecmode_ib], arg) == 0) {
      _set_vprec_mode(vprecmode_ib, ctx);
    } else if (interflop_strcasecmp(VPREC_MODE_STR[vprecmode_ob], arg) == 0) {
      _set_vprec_mode(vprecmode_ob, ctx);
    } else {
      logger_error("--%s invalid value provided, must be one of: "
                   "{ieee, full, ib, ob}.",
                   key_mode_str);
    }
    break;
  case KEY_ERR_MODE:
    /* vprec error mode */
    if (interflop_strcasecmp(VPREC_ERR_MODE_STR[vprec_err_mode_rel], arg) ==
        0) {
      _set_vprec_error_mode(vprec_err_mode_rel, ctx);
    } else if (interflop_strcasecmp(VPREC_ERR_MODE_STR[vprec_err_mode_abs],
                                    arg) == 0) {
      _set_vprec_error_mode(vprec_err_mode_abs, ctx);
    } else if (interflop_strcasecmp(VPREC_ERR_MODE_STR[vprec_err_mode_all],
                                    arg) == 0) {
      _set_vprec_error_mode(vprec_err_mode_all, ctx);
    } else {
      logger_error("--%s invalid value provided, must be one of: "
                   "{rel, abs, all}.",
                   key_err_mode_str);
    }
    break;
  case KEY_ERR_EXP:
    /* exponent of the maximum absolute error */
    error = 0;
    long exp = interflop_strtol(arg, &endptr, &error);
    if (error != 0) {
      logger_error("--%s invalid value provided, must be an integer",
                   key_err_exp_str);
    } else {
      _set_vprec_max_abs_err_exp(exp, ctx);
    }
    break;
  case KEY_DAZ:
    /* denormals-are-zero */
    _set_vprec_daz(true, ctx);
    break;
  case KEY_FTZ:
    /* flush-to-zero */
    _set_vprec_ftz(true, ctx);
    break;
  case KEY_PRESET:
    /* preset */
    if (interflop_strcmp(VPREC_PRESET_STR[vprec_preset_binary16], arg) == 0) {
      precision = vprec_preset_precision_binary16;
      range = vprec_preset_range_binary16;
    } else if (interflop_strcmp(VPREC_PRESET_STR[vprec_preset_binary32], arg) ==
               0) {
      precision = vprec_preset_precision_binary32;
      range = vprec_preset_range_binary32;
    } else if (interflop_strcmp(VPREC_PRESET_STR[vprec_preset_bfloat16], arg) ==
               0) {
      precision = vprec_preset_precision_bfloat16;
      range = vprec_preset_range_bfloat16;
    } else if (interflop_strcmp(VPREC_PRESET_STR[vprec_preset_tensorfloat],
                                arg) == 0) {
      precision = vprec_preset_precision_tensorfloat;
      range = vprec_preset_range_tensorfloat;
    } else if (interflop_strcmp(VPREC_PRESET_STR[vprec_preset_fp24], arg) ==
               0) {
      precision = vprec_preset_precision_fp24;
      range = vprec_preset_range_fp24;
    } else if (interflop_strcmp(VPREC_PRESET_STR[vprec_preset_PXR24], arg) ==
               0) {
      precision = vprec_preset_precision_PXR24;
      range = vprec_preset_range_PXR24;
    } else {
      logger_error("--%s invalid preset provided, must be one of: "
                   "{binary16, binary32, binary64, bfloat16, tensorfloat, "
                   "fp24, PXR24}",
                   key_preset_str);
      break;
    }

    /* set precision */
    _set_vprec_precision_binary32(precision, ctx);
    _set_vprec_precision_binary64(precision, ctx);

    /* set range */
    _set_vprec_range_binary32(range, ctx);
    _set_vprec_range_binary64(range, ctx);

    break;
  default:
    return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

static struct argp_child argpc[] = {
    {&vfi_argp, 0, "vprec function instrumentation", 0}, {0}};

static struct argp argp = {options, parse_opt, "", "", argpc, NULL, NULL};

void INTERFLOP_VPREC_API(cli)(int argc, char **argv, void *context) {
  /* parse backend arguments */
  vprec_context_t *ctx = (vprec_context_t *)context;
  if (interflop_argp_parse != NULL) {
    interflop_argp_parse(&argp, argc, argv, 0, 0, ctx);
  } else {
    interflop_panic("Interflop backend error: argp_parse not implemented\n"
                    "Provide implementation or use interflop_configure to "
                    "configure the backend\n");
  }
}

void INTERFLOP_VPREC_API(configure)(void *configure, void *context) {
  vprec_context_t *ctx = (vprec_context_t *)context;
  vprec_conf_t *conf = (vprec_conf_t *)configure;
  int precision_binary32 = (int)conf->precision_binary32;
  int precision_binary64 = (int)conf->precision_binary64;
  int range_binary32 = (int)conf->range_binary32;
  int range_binary64 = (int)conf->range_binary64;
  if (conf->preset != (unsigned int)(-1)) {
    precision_binary32 = _get_vprec_preset_precision(conf->preset);
    precision_binary64 = _get_vprec_preset_precision(conf->preset);
    range_binary32 = _get_vprec_preset_range(conf->preset);
    range_binary64 = _get_vprec_preset_range(conf->preset);
  }
  _set_vprec_precision_binary32(precision_binary32, ctx);
  _set_vprec_precision_binary64(precision_binary64, ctx);
  _set_vprec_range_binary32(range_binary32, ctx);
  _set_vprec_range_binary64(range_binary64, ctx);
  _set_vprec_mode(conf->mode, ctx);
  _set_vprec_error_mode(conf->err_mode, ctx);
  if (conf->max_abs_err_exponent != (unsigned int)(-1)) {
    _set_vprec_max_abs_err_exp(conf->max_abs_err_exponent, ctx);
  }
  if (conf->daz) {
    _set_vprec_daz(context, ctx);
  }
  if (conf->ftz) {
    _set_vprec_ftz(context, ctx);
  }
}

static void print_information_header(void *context) {
  /* Environnement variable to disable loading message */
  char *silent_load_env = interflop_getenv("VFC_BACKENDS_SILENT_LOAD");
  bool silent_load = ((silent_load_env == NULL) ||
                      (interflop_strcasecmp(silent_load_env, "True") != 0))
                         ? false
                         : true;

  if (silent_load) {
    return;
  }

  vprec_context_t *ctx = (vprec_context_t *)context;

  logger_info("load backend with: \n");
  logger_info("%s = %d\n", key_prec_b32_str, ctx->binary32_precision);
  logger_info("%s = %d\n", key_range_b32_str, ctx->binary32_range);
  logger_info("%s = %d\n", key_prec_b64_str, ctx->binary64_precision);
  logger_info("%s = %d\n", key_range_b64_str, ctx->binary64_range);
  logger_info("%s = %s\n", key_mode_str, VPREC_MODE_STR[ctx->mode]);
  logger_info("%s = %s\n", key_err_mode_str, _get_error_mode_str(ctx));
  logger_info("%s = %d\n", key_err_exp_str, ctx->absErr_exp);
  logger_info("%s = %s\n", key_daz_str, ctx->daz ? "true" : "false");
  logger_info("%s = %s\n", key_ftz_str, ctx->ftz ? "true" : "false");
  _vfi_print_information_header(context);
}

struct interflop_backend_interface_t INTERFLOP_VPREC_API(init)(void *context) {

  vprec_context_t *ctx = (vprec_context_t *)context;

  /* initialize vprec function instrumentation context */
  _vfi_init(ctx);

  struct interflop_backend_interface_t interflop_backend_vprec = {
      .interflop_add_float = INTERFLOP_VPREC_API(add_float),
      .interflop_sub_float = INTERFLOP_VPREC_API(sub_float),
      .interflop_mul_float = INTERFLOP_VPREC_API(mul_float),
      .interflop_div_float = INTERFLOP_VPREC_API(div_float),
      .interflop_cmp_float = NULL,
      .interflop_add_double = INTERFLOP_VPREC_API(add_double),
      .interflop_sub_double = INTERFLOP_VPREC_API(sub_double),
      .interflop_mul_double = INTERFLOP_VPREC_API(mul_double),
      .interflop_div_double = INTERFLOP_VPREC_API(div_double),
      .interflop_cmp_double = NULL,
      .interflop_cast_double_to_float =
          INTERFLOP_VPREC_API(cast_double_to_float),
      .interflop_fma_float = INTERFLOP_VPREC_API(fma_float),
      .interflop_fma_double = INTERFLOP_VPREC_API(fma_double),
      .interflop_enter_function = INTERFLOP_VPREC_API(enter_function),
      .interflop_exit_function = INTERFLOP_VPREC_API(exit_function),
      .interflop_user_call = INTERFLOP_VPREC_API(user_call),
      .interflop_finalize = INTERFLOP_VPREC_API(finalize)};

  print_information_header(ctx);

  return interflop_backend_vprec;
}

struct interflop_backend_interface_t interflop_init(void *context)
    __attribute__((weak, alias("interflop_vprec_init")));

void interflop_pre_init(interflop_panic_t panic, File *stream, void **context)
    __attribute__((weak, alias("interflop_vprec_pre_init")));

void interflop_cli(int argc, char **argv, void *context)
    __attribute__((weak, alias("interflop_vprec_cli")));
