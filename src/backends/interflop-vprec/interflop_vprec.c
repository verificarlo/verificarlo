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
#include <string.h>
#include <strings.h>

#include "../../common/float_const.h"
#include "../../common/float_struct.h"
#include "../../common/float_utils.h"
#include "../../common/interflop.h"
#include "../../common/logger.h"
#include "../../common/vfc_hashmap.h"
#include "../../common/vprec_tools.h"

typedef enum {
  KEY_PREC_B32,
  KEY_PREC_B64,
  KEY_RANGE_B32,
  KEY_RANGE_B64,
  KEY_ERR_EXP,
  KEY_INPUT_FILE,
  KEY_OUTPUT_FILE,
  KEY_MODE = 'm',
  KEY_ERR_MODE = 'e',
  KEY_INSTRUMENT = 'i',
  KEY_DAZ = 'd',
  KEY_FTZ = 'f'
} key_args;

static const char key_prec_b32_str[] = "precision-binary32";
static const char key_prec_b64_str[] = "precision-binary64";
static const char key_range_b32_str[] = "range-binary32";
static const char key_range_b64_str[] = "range-binary64";
static const char key_input_file_str[] = "prec-input-file";
static const char key_output_file_str[] = "prec-output-file";
static const char key_mode_str[] = "mode";
static const char key_err_mode_str[] = "error-mode";
static const char key_err_exp_str[] = "max-abs-error-exponent";
static const char key_instrument_str[] = "instrument";
static const char key_daz_str[] = "daz";
static const char key_ftz_str[] = "ftz";

typedef struct {
  bool relErr;
  bool absErr;
  int absErr_exp;
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

/* define the available error modes */
typedef enum {
  vprec_err_mode_rel,
  vprec_err_mode_abs,
  vprec_err_mode_all,
  _vprec_err_mode_end_
} vprec_err_mode;

static const char *VPREC_ERR_MODE_STR[] = {"rel", "abs", "all"};

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
#define VPREC_RANGE_BINARY32_MIN 2
#define VPREC_RANGE_BINARY32_MAX FLOAT_EXP_SIZE
#define VPREC_RANGE_BINARY32_DEFAULT FLOAT_EXP_SIZE

/* default values of precision and range for binary64 */
#define VPREC_PRECISION_BINARY64_MIN 1
#define VPREC_PRECISION_BINARY64_MAX DOUBLE_PMAN_SIZE
#define VPREC_PRECISION_BINARY64_DEFAULT DOUBLE_PMAN_SIZE
#define VPREC_RANGE_BINARY64_MIN 2
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

/* variables and structure for instrumentation mode */

/* define instrumentation modes */
typedef enum {
  vprecinst_arg,
  vprecinst_op,
  vprecinst_all,
  vprecinst_none,
  _vprecinst_end_
} vprec_inst_mode;

/* default instrumentation mode */
#define VPREC_INST_MODE_DEFAULT vprecinst_none

static const char *vprec_input_file = NULL;
static const char *vprec_output_file = NULL;
static vprec_inst_mode VPREC_INST_MODE = VPREC_INST_MODE_DEFAULT;

/* instrumentation modes' names */
static const char *VPREC_INST_MODE_STR[] = {"arguments", "operations", "all",
                                            "none"};

/******************** VPREC CONTROL FUNCTIONS *******************
 * The following functions are used to set virtual precision,
 * VPREC mode of operation and instrumentation mode.
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

void _set_vprec_input_file(const char *input_file) {
  vprec_input_file = input_file;
}

void _set_vprec_output_file(const char *output_file) {
  vprec_output_file = output_file;
}

void _set_vprec_inst_mode(vprec_inst_mode mode) {
  if (mode >= _vprecinst_end_) {
    logger_error("invalid instrumentation mode provided, must be one of:"
                 "{arguments, operations, all, none}.");
  } else {
    VPREC_INST_MODE = mode;
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

// Round the float with the given precision
static float _vprec_round_binary32(float a, char is_input, void *context,
                                   int binary32_range, int binary32_precision) {
  if (!isfinite(a)) {
    return a;
  }

  /* round to zero or set to infinity if underflow or overflow compared to
   * VPRECLIB_BINARY32_RANGE */
  int emax = (1 << (binary32_range - 1)) - 1;
  // here emin is the smallest exponent in the *normal* range
  int emin = 1 - emax;

  /* in absolute error mode, the error threshold also gives the possible
   * underflow limit */
  if ((((t_context *)context)->relErr == true) &&
      (((t_context *)context)->absErr == true)) {
    if (((t_context *)context)->absErr_exp > emin)
      emin = t_context->absErr_exp;
  }

  binary32 aexp = {.f32 = a};
  aexp.s32 = ((FLOAT_GET_EXP & aexp.u32) >> FLOAT_PMAN_SIZE) - FLOAT_EXP_COMP;

  /* check for overflow or underflow in target range */
  if (aexp.s32 > emax) {
    a = a * INFINITY;
    sp_case = true;
  }

  if ((((t_context *)context)->daz && is_input) ||
      (((t_context *)context)->ftz && !is_input)) {
    a = 0;
  } else {
    if ((((t_context *)context)->relErr == true) &&
        (((t_context *)context)->absErr == true)) {
      /* vprec error mode all */
      if ((-1) * ((t_context *)context)->absErr_exp < binary32_precision)
        a = handle_binary32_denormal(a, emin, aexp.u32,
                                     (-1) * ((t_context *)context)->absErr_exp);
      else
        a = handle_binary32_denormal(a, emin, aexp.u32, binary32_precision);
    } else if (((t_context *)context)->absErr == true) {
      /* vprec error mode abs */
      a = handle_binary32_denormal(a, emin, aexp.u32,
                                   (-1) * ((t_context *)context)->absErr_exp);
    }
  }

  /* Specials ops must be placed after denormal handling  */
  /* If one of the operand raises an underflow, the operation */
  /* has a different behavior. Example: x*Inf != 0*Inf */

  if (sp_case) {
    return a;
  }

  /* else, normal case: can be executed even if a
     previously rounded and truncated as denormal */
  if (binary32_precision < FLOAT_PMAN_SIZE) {
    if ((((t_context *)context)->relErr == true) &&
        (((t_context *)context)->absErr == true)) {
      /* vprec error mode all */
      if ((-1) * ((t_context *)context)->absErr_exp < binary32_precision)
        a = round_binary32_normal(a, (-1) * ((t_context *)context)->absErr_exp);
      else
        a = round_binary32_normal(a, binary32_precision);
    } else if (((t_context *)context)->absErr == true) {
      /* vprec error mode abs */
      a = round_binary32_normal(a, (-1) * ((t_context *)context)->absErr_exp);
    }
  } else {
    a = round_binary32_normal(a, binary32_precision);
  }

  return a;
}

// Round the double with the given precision
static double _vprec_round_binary64(double a, char is_input, void *context,
                                    int binary64_range,
                                    int binary64_precision) {
  /* test if a or b are special cases */
  if (!isfinite(a)) {
    return a;
  }

  /* round to zero or set to infinity if underflow or overflow compare to
   * VPRECLIB_BINARY64_RANGE */
  int emax = (1 << (binary64_range - 1)) - 1;
  // here emin is the smallest exponent in the *normal* range
  int emin = 1 - emax;

  /* in absolute error mode, the error threshold also gives the possible
   * underflow limit */
  if ((((t_context *)context)->relErr == true) &&
      (((t_context *)context)->absErr == true)) {
    if (((t_context *)context)->absErr_exp > emin)
      emin = ((t_context *)context)->absErr_exp;
  }

  binary64 aexp = {.f64 = a};
  aexp.s64 =
      ((DOUBLE_GET_EXP & aexp.u64) >> DOUBLE_PMAN_SIZE) - DOUBLE_EXP_COMP;

  /* check for overflow or underflow in target range */
  if (aexp.s64 > emax) {
    a = a * INFINITY;
    sp_case = true;
  }

  if (aexp.s64 <= emin) {
    if ((((t_context *)context)->relErr == true) &&
        (((t_context *)context)->absErr == true)) {
      /* vprec error mode all */
      if ((-1) * ((t_context *)context)->absErr_exp < binary64_precision)
        a = handle_binary64_denormal(a, emin, aexp.u64,
                                     (-1) * ((t_context *)context)->absErr_exp);
      else
        a = handle_binary64_denormal(a, emin, aexp.u64, binary64_precision);
    } else if (((t_context *)context)->absErr == true) {
      /* vprec error mode abs */
      a = handle_binary64_denormal(a, emin, aexp.u64,
                                   (-1) * ((t_context *)context)->absErr_exp);
    }
  }

  /* Special ops must be placed after denormal handling  */
  /* If the operand raises an underflow, the operation */
  /* has a different behavior. Example: x*Inf != 0*Inf */
  if (sp_case) {
    return a;
  }

  /* else normal case, can be executed even if a previously rounded and
   * truncated as denormal */
  if (binary64_precision < DOUBLE_PMAN_SIZE) {
    if ((((t_context *)context)->relErr == true) &&
        (((t_context *)context)->absErr == true)) {
      /* vprec error mode all */
      if ((-1) * ((t_context *)context)->absErr_exp < binary64_precision)
        a = round_binary64_normal(a, (-1) * ((t_context *)context)->absErr_exp);
      else
        a = round_binary64_normal(a, binary64_precision);
    } else if (((t_context *)context)->absErr == true) {
      /* vprec error mode abs */
      a = round_binary64_normal(a, (-1) * ((t_context *)context)->absErr_exp);
    }
  } else {
    a = round_binary64_normal(a, binary64_precision);
  }

  return a;
}

static inline float _vprec_binary32_binary_op(float a, float b,
                                              const vprec_operation op,
                                              void *context) {
  float res = 0;

  if ((VPRECLIB_MODE == vprecmode_full) || (VPRECLIB_MODE == vprecmode_ib)) {
    a = _vprec_round_binary32(a, 1, context, VPRECLIB_BINARY32_RANGE,
                              VPRECLIB_BINARY32_PRECISION);
    b = _vprec_round_binary32(b, 1, context, VPRECLIB_BINARY32_RANGE,
                              VPRECLIB_BINARY32_PRECISION);
  }

  perform_binary_op(op, res, a, b);

  if ((VPRECLIB_MODE == vprecmode_full) || (VPRECLIB_MODE == vprecmode_ob)) {
    res = _vprec_round_binary32(res, 0, context, VPRECLIB_BINARY32_RANGE,
                                VPRECLIB_BINARY32_PRECISION);
  }

  return res;
}

static inline double _vprec_binary64_binary_op(double a, double b,
                                               const vprec_operation op,
                                               void *context) {
  double res = 0;

  if ((VPRECLIB_MODE == vprecmode_full) || (VPRECLIB_MODE == vprecmode_ib)) {
    a = _vprec_round_binary64(a, 1, context, VPRECLIB_BINARY64_RANGE,
                              VPRECLIB_BINARY64_PRECISION);
    b = _vprec_round_binary64(b, 1, context, VPRECLIB_BINARY64_RANGE,
                              VPRECLIB_BINARY64_PRECISION);
  }

  perform_binary_op(op, res, a, b);

  if ((VPRECLIB_MODE == vprecmode_full) || (VPRECLIB_MODE == vprecmode_ob)) {
    res = _vprec_round_binary64(res, 0, context, VPRECLIB_BINARY64_RANGE,
                                VPRECLIB_BINARY64_PRECISION);
  }

  return res;
}

/******************** VPREC INSTRUMENTATION FUNCTIONS ********************
 * The following set of functions is used to apply vprec on instrumented
 * functions. For that we need a hashmap to stock data and reading and
 * writing functions to get and save them. Enter and exit functions are
 * called before and after the instrumented function and allow us to set
 * the desired precision or to round arguments, depending on the mode.
 *************************************************************************/

vfc_hashmap_t _vprec_func_map;

/* type (4 bits) | range (6 bits) | precision (6 bits) */
typedef unsigned short _vprec_func_precision_t;

/* last 4 bits are for the type concerned by this precision */
_vprec_func_precision_t
get_vprec_func_precision_type(const _vprec_func_precision_t prec) {
  return (prec & 0xF000) >> 12;
}

/* next 6 bits are for the exponent precision */
_vprec_func_precision_t
get_vprec_func_precision_exponent(const _vprec_func_precision_t prec) {
  return (prec & 0xFC0) >> 6;
}

/* first 6 bits are for the mantissa precision */
_vprec_func_precision_t
get_vprec_func_precision_mantissa(const _vprec_func_precision_t prec) {
  return prec & 0x3F;
}

_vprec_func_precision_t
set_vprec_func_precision(_vprec_func_precision_t type,
                         _vprec_func_precision_t range,
                         _vprec_func_precision_t precision) {
  if (type >= FTYPES_END) {
    logger_error("given types is not managed by function instrumentation: %hd",
                 type);
  }
  if ((range > VPREC_RANGE_BINARY32_MAX || range < VPREC_RANGE_BINARY32_MIN) &&
      type == FFLOAT) {
    logger_error("invalid range for binary 32: %hd", range);
  }
  if ((precision > VPREC_PRECISION_BINARY32_MAX ||
       precision < VPREC_PRECISION_BINARY32_MIN) &&
      type == FFLOAT) {
    logger_error("invalid precision for binary 32: %hd", precision);
  }
  if ((range > VPREC_RANGE_BINARY64_MAX || range < VPREC_RANGE_BINARY64_MIN) &&
      type == FDOUBLE) {
    logger_error("invalid range for binary 64: %hd", range);
  }
  if ((precision > VPREC_PRECISION_BINARY64_MAX ||
       precision < VPREC_PRECISION_BINARY64_MIN) &&
      type == FDOUBLE) {
    logger_error("invalid precision for binary 64: %hd", precision);
  }
  _vprec_func_precision_t prec = (type << 12) | (range << 6) | precision;
  return prec;
}

typedef struct _vprec_inst_function {
  // id of the function
  char id[500];
  // internal precision for 32 bit float operations
  _vprec_func_precision_t precision_binary32;
  // internal precision for 64 bit float operations
  _vprec_func_precision_t precision_binary64;
  // precisions for floating point input arguments
  _vprec_func_precision_t *input_arguments;
  // precisions for floating point ouput arguments
  _vprec_func_precision_t *output_arguments;
  // number of floating point input arguments
  int nb_input_args;
  // number of floating point output arguments
  int nb_output_args;
  // number of call for this call site
  int n_calls;
} _vprec_inst_function_t;

void _vprec_read_hasmap(FILE *fin) {
  _vprec_inst_function_t function;
  int binary64_precision, binary64_range, binary32_precision, binary32_range,
      type;

  while (fscanf(fin, "%s\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n", function.id,
                &binary64_precision, &binary64_range, &binary32_precision,
                &binary32_range, &function.nb_input_args,
                &function.nb_output_args, &function.n_calls) == 8) {
    // set the internal precision for 64 bit floating point operations
    function.precision_binary64 =
        set_vprec_func_precision(FDOUBLE, binary64_range, binary64_precision);
    // set the internal precision for 32 bit floating point operations
    function.precision_binary32 =
        set_vprec_func_precision(FFLOAT, binary32_range, binary32_precision);
    // allocate space for input arguments
    function.input_arguments =
        malloc(function.nb_input_args * sizeof(_vprec_func_precision_t));
    // allocate space for output arguments
    function.output_arguments =
        malloc(function.nb_output_args * sizeof(_vprec_func_precision_t));

    // get input arguments precision
    for (int i = 0; i < function.nb_input_args; i++) {
      if (fscanf(fin, "input:\t%d\t%d\t%d\n", &type, &binary64_precision,
                 &binary64_range)) {
        function.input_arguments[i] =
            set_vprec_func_precision(type, binary64_range, binary64_precision);
      } else {
        break;
      }
    }

    // get output arguments precision
    for (int i = 0; i < function.nb_output_args; i++) {
      if (fscanf(fin, "output:\t%d\t%d\t%d\n", &type, &binary64_precision,
                 &binary64_range)) {
        function.output_arguments[i] =
            set_vprec_func_precision(type, binary64_range, binary64_precision);
      } else {
        break;
      }
    }

    // insert in the hashmap
    _vprec_inst_function_t *adress = malloc(sizeof(_vprec_inst_function_t));
    (*adress) = function;
    vfc_hashmap_insert(_vprec_func_map, vfc_hashmap_str_function(function.id),
                       adress);
  }
}

void _vprec_write_hasmap(FILE *fout) {
  for (int ii = 0; ii < _vprec_func_map->capacity; ii++) {
    if (get_value_at(_vprec_func_map->items, ii) != 0 &&
        get_value_at(_vprec_func_map->items, ii) != 0) {
      _vprec_inst_function_t *function =
          (_vprec_inst_function_t *)get_value_at(_vprec_func_map->items, ii);

      fprintf(fout, "%s\t%hu\t%hu\t%hu\t%hu\t%d\t%d\t%d\n", function->id,
              get_vprec_func_precision_mantissa(function->precision_binary64),
              get_vprec_func_precision_exponent(function->precision_binary64),
              get_vprec_func_precision_mantissa(function->precision_binary32),
              get_vprec_func_precision_exponent(function->precision_binary32),
              function->nb_input_args, function->nb_output_args,
              function->n_calls);
      for (int i = 0; i < function->nb_input_args; i++) {
        fprintf(
            fout, "input:\t%hu\t%hu\t%hu\n",
            get_vprec_func_precision_type(function->input_arguments[i]),
            get_vprec_func_precision_mantissa(function->input_arguments[i]),
            get_vprec_func_precision_exponent(function->input_arguments[i]));
      }
      for (int i = 0; i < function->nb_output_args; i++) {
        fprintf(
            fout, "output:\t%hu\t%hu\t%hu\n",
            get_vprec_func_precision_type(function->output_arguments[i]),
            get_vprec_func_precision_mantissa(function->output_arguments[i]),
            get_vprec_func_precision_exponent(function->output_arguments[i]));
      }
    }
  }
}

void _interflop_enter_function(interflop_function_stack_t *stack, void *context,
                               int nb_args, va_list ap) {
  interflop_function_info_t *function_info = stack->array[stack->top];

  if (function_info == NULL)
    logger_error("Call stack error\n");

  _vprec_inst_function_t *function_inst = vfc_hashmap_get(
      _vprec_func_map, vfc_hashmap_str_function(function_info->id));

  // if the function is not in the hashtable
  if (function_inst == NULL) {
    function_inst = malloc(sizeof(_vprec_inst_function_t));

    // initialize the structure
    strcpy(function_inst->id, function_info->id);
    function_inst->precision_binary64 =
        set_vprec_func_precision(FDOUBLE, VPREC_RANGE_BINARY64_DEFAULT,
                                 VPREC_PRECISION_BINARY64_DEFAULT);
    function_inst->precision_binary32 = set_vprec_func_precision(
        FFLOAT, VPREC_RANGE_BINARY32_DEFAULT, VPREC_PRECISION_BINARY32_DEFAULT);
    function_inst->nb_input_args = 0;
    function_inst->nb_output_args = 0;
    function_inst->input_arguments = NULL;
    function_inst->output_arguments = NULL;
    function_inst->n_calls = 0;

    // insert the function in the hashmap
    vfc_hashmap_insert(_vprec_func_map,
                       vfc_hashmap_str_function(function_info->id),
                       function_inst);
  }

  // increment the number of calls
  function_inst->n_calls++;

  // set precision with custom values depending on the mode
  if (!function_info->isLibraryFunction &&
      !function_info->isIntrinsicFunction && VPREC_INST_MODE != vprecinst_arg &&
      VPREC_INST_MODE != vprecinst_none) {
    _set_vprec_precision_binary64(
        get_vprec_func_precision_mantissa(function_inst->precision_binary64));
    _set_vprec_range_binary64(
        get_vprec_func_precision_exponent(function_inst->precision_binary64));
    _set_vprec_precision_binary32(
        get_vprec_func_precision_mantissa(function_inst->precision_binary32));
    _set_vprec_range_binary32(
        get_vprec_func_precision_exponent(function_inst->precision_binary32));
  }

  // if input arguments are not in the structure
  if (function_inst->input_arguments == NULL && nb_args > 0) {
    function_inst->input_arguments =
        malloc(sizeof(_vprec_func_precision_t) * nb_args);
    function_inst->nb_input_args = nb_args;

    for (int i = 0; i < nb_args; i++) {
      int type = va_arg(ap, int);
      void *value = va_arg(ap, void *);

      if (type == FDOUBLE) {
        function_inst->input_arguments[i] =
            set_vprec_func_precision(FDOUBLE, VPREC_RANGE_BINARY64_DEFAULT,
                                     VPREC_PRECISION_BINARY64_DEFAULT);
      } else if (type == FFLOAT) {
        function_inst->input_arguments[i] =
            set_vprec_func_precision(FFLOAT, VPREC_RANGE_BINARY32_DEFAULT,
                                     VPREC_PRECISION_BINARY32_DEFAULT);
      }
    }

    // round to default value is useless, so exit
    return;
  }

  // set precision with custom values depending on the mode
  if (((VPRECLIB_MODE == vprecmode_full) || (VPRECLIB_MODE == vprecmode_ib)) &&
      ((VPREC_INST_MODE == vprecinst_all) ||
       (VPREC_INST_MODE == vprecinst_arg)) &&
      VPREC_INST_MODE != vprecinst_none) {
    for (int i = 0; i < nb_args; i++) {
      int type = va_arg(ap, int);

      if (type == FDOUBLE) {
        double *value = va_arg(ap, double *);
        *value = _vprec_round_binary64(*value, 1, context,
                                       get_vprec_func_precision_exponent(
                                           function_inst->input_arguments[i]),
                                       get_vprec_func_precision_mantissa(
                                           function_inst->input_arguments[i]));
      } else if (type == FFLOAT) {
        float *value = va_arg(ap, float *);
        *value = _vprec_round_binary32(*value, 1, context,
                                       get_vprec_func_precision_exponent(
                                           function_inst->input_arguments[i]),
                                       get_vprec_func_precision_mantissa(
                                           function_inst->input_arguments[i]));
      }
    }
  }
}

void _interflop_exit_function(interflop_function_stack_t *stack, void *context,
                              int nb_args, va_list ap) {
  interflop_function_info_t *function_info = stack->array[stack->top];

  if (function_info == NULL)
    logger_error("Call stack error \n");

  _vprec_inst_function_t *function_inst = vfc_hashmap_get(
      _vprec_func_map, vfc_hashmap_str_function(function_info->id));

  if (stack->array[stack->top + 1] != NULL) {
    interflop_function_info_t *parent_info = stack->array[stack->top + 1];

    if (!parent_info->isLibraryFunction && !parent_info->isIntrinsicFunction &&
        VPREC_INST_MODE != vprecinst_arg && VPREC_INST_MODE != vprecinst_none) {

      _vprec_inst_function_t *function_parent = vfc_hashmap_get(
          _vprec_func_map, vfc_hashmap_str_function(parent_info->id));

      if (function_parent != NULL) {
        _set_vprec_precision_binary64(get_vprec_func_precision_mantissa(
            function_parent->precision_binary64));
        _set_vprec_range_binary64(get_vprec_func_precision_exponent(
            function_parent->precision_binary64));
        _set_vprec_precision_binary32(get_vprec_func_precision_mantissa(
            function_parent->precision_binary32));
        _set_vprec_range_binary32(get_vprec_func_precision_exponent(
            function_parent->precision_binary32));
      }
    }
  }

  // if output arguments are not in the structure
  if (function_inst->output_arguments == NULL && nb_args > 0) {
    function_inst->output_arguments =
        malloc(sizeof(_vprec_func_precision_t) * nb_args);
    function_inst->nb_output_args = nb_args;

    for (int i = 0; i < nb_args; i++) {
      int type = va_arg(ap, int);
      void *value = va_arg(ap, void *);

      if (type == FDOUBLE) {
        function_inst->output_arguments[i] =
            set_vprec_func_precision(FDOUBLE, VPREC_RANGE_BINARY64_DEFAULT,
                                     VPREC_PRECISION_BINARY64_DEFAULT);
      } else if (type == FFLOAT) {
        function_inst->output_arguments[i] =
            set_vprec_func_precision(FFLOAT, VPREC_RANGE_BINARY32_DEFAULT,
                                     VPREC_PRECISION_BINARY32_DEFAULT);
      }
    }

    // round to default value is useless, so exit
    return;
  }

  // set precision with custom values depending on the mode
  if (VPREC_INST_MODE != vprecinst_none) {
    if (((VPRECLIB_MODE == vprecmode_full) ||
         (VPRECLIB_MODE == vprecmode_ob)) &&
        (VPREC_INST_MODE == vprecinst_all ||
         VPREC_INST_MODE == vprecinst_arg)) {
      for (int i = 0; i < nb_args; i++) {
        int type = va_arg(ap, int);

        if (type == FDOUBLE) {
          double *value = va_arg(ap, double *);
          *value =
              _vprec_round_binary64(*value, 0, context,
                                    get_vprec_func_precision_exponent(
                                        function_inst->output_arguments[i]),
                                    get_vprec_func_precision_mantissa(
                                        function_inst->output_arguments[i]));
        } else if (type == FFLOAT) {
          float *value = va_arg(ap, float *);
          *value =
              _vprec_round_binary32(*value, 0, context,
                                    get_vprec_func_precision_exponent(
                                        function_inst->output_arguments[i]),
                                    get_vprec_func_precision_mantissa(
                                        function_inst->output_arguments[i]));
        }
      }
    }
  }
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
     "select precision for binary32 (PRECISION >= 0)", 0},
    {key_prec_b64_str, KEY_PREC_B64, "PRECISION", 0,
     "select precision for binary64 (PRECISION >= 0)", 0},
    {key_range_b32_str, KEY_RANGE_B32, "RANGE", 0,
     "select range for binary32 (0 < RANGE && RANGE <= 8)", 0},
    {key_range_b64_str, KEY_RANGE_B64, "RANGE", 0,
     "select range for binary64 (0 < RANGE && RANGE <= 11)", 0},
    {key_input_file_str, KEY_INPUT_FILE, "INPUT", 0,
     "input file with the precision configuration to use", 0},
    {key_output_file_str, KEY_OUTPUT_FILE, "OUTPUT", 0,
     "output file where the precision profile is written", 0},
    {key_mode_str, KEY_MODE, "MODE", 0,
     "select VPREC mode among {ieee, full, ib, ob}", 0},
    {key_err_mode_str, KEY_ERR_MODE, "ERROR_MODE", 0,
     "select error mode among {rel, abs, all}", 0},
    {key_err_exp_str, KEY_ERR_EXP, "MAX_ABS_ERROR_EXPONENT", 0,
     "select magnitude of the maximum absolute error", 0},
    {key_instrument_str, KEY_INSTRUMENT, "INSTRUMENTATION", 0,
     "select VPREC instrumentation mode among {arguments, operations, full}",
     0},
    {key_daz_str, KEY_DAZ, 0, 0,
     "denormals-are-zero: sets denormals inputs to zero", 0},
    {key_ftz_str, KEY_FTZ, 0, 0, "flush-to-zero: sets denormal output to zero",
     0},
    {0}};

//
// prec-output-file

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
  case KEY_INPUT_FILE:
    /* input file */
    _set_vprec_input_file(arg);
    break;
  case KEY_OUTPUT_FILE:
    /* output file */
    _set_vprec_output_file(arg);
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
  case KEY_ERR_MODE:
    /* vprec error mode */
    if (strcasecmp(VPREC_ERR_MODE_STR[vprec_err_mode_rel], arg) == 0) {
      ctx->relErr = true;
      ctx->absErr = false;
    } else if (strcasecmp(VPREC_ERR_MODE_STR[vprec_err_mode_abs], arg) == 0) {
      ctx->relErr = false;
      ctx->absErr = true;
    } else if (strcasecmp(VPREC_ERR_MODE_STR[vprec_err_mode_all], arg) == 0) {
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
  case KEY_INSTRUMENT:
    /* instrumentation mode */
    if (strcasecmp(VPREC_INST_MODE_STR[vprecinst_arg], arg) == 0) {
      _set_vprec_inst_mode(vprecinst_arg);
    } else if (strcasecmp(VPREC_INST_MODE_STR[vprecinst_op], arg) == 0) {
      _set_vprec_inst_mode(vprecinst_op);
    } else if (strcasecmp(VPREC_INST_MODE_STR[vprecinst_all], arg) == 0) {
      _set_vprec_inst_mode(vprecinst_all);
    } else if (strcasecmp(VPREC_INST_MODE_STR[vprecinst_none], arg) == 0) {
      _set_vprec_inst_mode(vprecinst_none);
    } else {
      logger_error("--%s invalid value provided, must be one of: "
                   "{arguments, operations, all}.",
                   key_instrument_str);
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

static struct argp argp = {options, parse_opt, "", "", NULL, NULL, NULL};

void init_context(t_context *ctx) {
  ctx->relErr = true;
  ctx->absErr = false;
  ctx->absErr_exp = 112;
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
      "%s = %s, "
      "%s = %d, "
      "%s = %s, "
      "%s = %s and "
      "%s = %s"
      "\n",
      key_prec_b32_str, VPRECLIB_BINARY32_PRECISION, key_range_b32_str,
      VPRECLIB_BINARY32_RANGE, key_prec_b64_str, VPRECLIB_BINARY64_PRECISION,
      key_range_b64_str, VPRECLIB_BINARY64_RANGE, key_mode_str,
      VPREC_MODE_STR[VPRECLIB_MODE], key_err_mode_str,
      (ctx->relErr && !ctx->absErr)
          ? VPREC_ERR_MODE_STR[vprec_err_mode_rel]
          : (!ctx->relErr && ctx->absErr)
                ? VPREC_ERR_MODE_STR[vprec_err_mode_abs]
                : (ctx->relErr && ctx->absErr)
                      ? VPREC_ERR_MODE_STR[vprec_err_mode_all]
                      : VPREC_ERR_MODE_STR[vprec_err_mode_rel],
      key_err_exp_str, (ctx->absErr_exp), key_daz_str,
      ctx->daz ? "true" : "false", key_ftz_str, ctx->ftz ? "true" : "false",
      key_instrument_str, VPREC_INST_MODE_STR[VPREC_INST_MODE]);
}

void _interflop_finalize(void *context) {
  /* save the hashmap */
  if (vprec_output_file != NULL) {
    FILE *f = fopen(vprec_output_file, "w");
    if (f != NULL) {
      _vprec_write_hasmap(f);
      fclose(f);
    } else {
      logger_error("Output file can't be written");
    }
  }
  /* free vprec_function_map */
  vfc_hashmap_free(_vprec_func_map);

  /* destroy vprec_function_map */
  vfc_hashmap_destroy(_vprec_func_map);
}

struct interflop_backend_interface_t interflop_init(int argc, char **argv,
                                                    void **context) {

  /* Initialize the logger */
  logger_init();

  /* Initialize the vprec_function_map */
  _vprec_func_map = vfc_hashmap_create();

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

  /* read the hashmap */
  if (vprec_input_file != NULL) {
    FILE *f = fopen(vprec_input_file, "r");
    if (f != NULL) {
      _vprec_read_hasmap(f);
      fclose(f);
    } else {
      logger_error("Input file can't be found");
    }
  }

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
      NULL,
      _interflop_enter_function,
      _interflop_exit_function,
      _interflop_finalize};

  return interflop_backend_vprec;
}
