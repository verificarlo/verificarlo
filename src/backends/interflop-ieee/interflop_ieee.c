/*****************************************************************************
 *                                                                           *
 *  This file is part of Verificarlo.                                        *
 *                                                                           *
 *  Copyright (c) 2015-2020                                                  *
 *     Verificarlo contributors                                              *
 *     Universite de Versailles St-Quentin-en-Yvelines                       *
 *     CMLA, Ecole Normale Superieure de Cachan                              *
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
#include <ieee754.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../common/float_const.h"
#include "../../common/interflop.h"
#include "../../common/logger.h"
#include "../../common/printf_specifier.h"

typedef enum {
  KEY_DEBUG = 'd',
  KEY_DEBUG_BINARY = 'b',
  KEY_NO_BACKEND_NAME = 's',
  KEY_PRINT_NEW_LINE = 'n',
  KEY_PRINT_SUBNORMAL_NORMALIZED
} key_args;

static const char key_debug_str[] = "debug";
static const char key_debug_binary_str[] = "debug-binary";
static const char key_no_backend_name_str[] = "no-backend-name";
static const char key_print_new_line_str[] = "print-new-line";
static const char key_print_subnormal_normalized_str[] =
    "print-subnormal-normalized";

typedef struct {
  bool debug;
  bool debug_binary;
  bool no_backend_name;
  bool print_new_line;
  bool print_subnormal_normalized;
} t_context;

typedef enum {
  ARITHMETIC = 0,
  COMPARISON = 1,
} operation_type;

#define STRING_MAX 256

#define FMT(X) _Generic(X, float : "b", double : "lb")
#define FMT_SUBNORMAL_NORMALIZED(X) _Generic(X, float : "#b", double : "#lb")

/* inserts the string <str_to_add> at position i */
/* increments i by the size of str_to_add */
void insert_string(char *dst, char *str_to_add, int *i) {
  int j = 0;
  do {
    dst[*i] = str_to_add[j];
  } while ((*i)++, j++, str_to_add[j] != '\0');
}

/* Auxiliary function to debug print that prints  */
/* a new line if requested by option --print-new-line  */
void debug_print_aux(void *context, char *fmt, va_list argp) {
  vfprintf(stderr, fmt, argp);
  if (((t_context *)context)->print_new_line) {
    fprintf(stderr, "\n");
  }
}

/* Debug print which replace %g by specific binary format %b */
/* if option --debug-binary is set */
void debug_print(void *context, char *fmt_flt, char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  if (((t_context *)context)->debug) {
    debug_print_aux(context, fmt, ap);
  } else {
    char new_fmt[STRING_MAX] = "";
    int i = 0, j = 0;
    do {
      switch (fmt[i]) {
      case 'g':
        insert_string(new_fmt, fmt_flt, &j);
        break;
      default:
        new_fmt[j] = fmt[i];
        j++;
        break;
      }
    } while (fmt[i++] != '\0');
    new_fmt[j] = '\0';
    debug_print_aux(context, new_fmt, ap);
  }
  va_end(ap);
}

#define DEBUG_HEADER "Decimal "
#define DEBUG_BINARY_HEADER "Binary "

/* This macro print the debug information for a, b and c */
/* the debug_print function handles automatically the format */
/* (decimal or binary) depending on the context */
#define DEBUG_PRINT(context, typeop, op, a, b, c)                              \
  {                                                                            \
    bool debug = ((t_context *)context)->debug ? true : false;                 \
    bool debug_binary = ((t_context *)context)->debug_binary ? true : false;   \
    bool subnormal_normalized =                                                \
        ((t_context *)context)->print_subnormal_normalized ? true : false;     \
    if (debug || debug_binary) {                                               \
      bool print_header =                                                      \
          (((t_context *)context)->no_backend_name) ? false : true;            \
      char *header = (debug) ? DEBUG_HEADER : DEBUG_BINARY_HEADER;             \
      char *float_fmt =                                                        \
          (subnormal_normalized) ? FMT_SUBNORMAL_NORMALIZED(a) : FMT(a);       \
      if (print_header) {                                                      \
        if (((t_context *)context)->print_new_line)                            \
          logger_info("%s\n", header);                                         \
        else                                                                   \
          logger_info("%s", header);                                           \
      }                                                                        \
      if (typeop == ARITHMETIC) {                                              \
        debug_print(context, float_fmt, "%g %s ", a, a, op);                   \
        debug_print(context, float_fmt, "%g -> ", b);                          \
        debug_print(context, float_fmt, "%g\n", c);                            \
      } else {                                                                 \
        debug_print(context, float_fmt, "%g [%s] ", a, op);                    \
        debug_print(context, float_fmt, "%g -> %s\n", b,                       \
                    c ? "true" : "false");                                     \
      }                                                                        \
    }                                                                          \
  }

static inline void debug_print_float(void *context, const operation_type typeop,
                                     const char *op, const float a,
                                     const float b, const float c) {
  DEBUG_PRINT(context, typeop, op, a, b, c);
}

static inline void debug_print_double(void *context,
                                      const operation_type typeop,
                                      const char *op, const double a,
                                      const double b, const double c) {
  DEBUG_PRINT(context, typeop, op, a, b, c);
}

/* Set C to the result of the comparison P between A and B */
/* Set STR to name of the comparison  */
#define SELECT_FLOAT_CMP(A, B, C, P, STR)                                      \
  switch (P) {                                                                 \
  case FCMP_FALSE:                                                             \
    *C = false;                                                                \
    STR = "FCMP_FALSE";                                                        \
    break;                                                                     \
  case FCMP_OEQ:                                                               \
    *C = ((A) == (B));                                                         \
    STR = "FCMP_OEQ";                                                          \
    break;                                                                     \
  case FCMP_OGT:                                                               \
    *C = isgreater(A, B);                                                      \
    STR = "FCMP_OGT";                                                          \
    break;                                                                     \
  case FCMP_OGE:                                                               \
    *C = isgreaterequal(A, B);                                                 \
    STR = "FCMP_OGE";                                                          \
    break;                                                                     \
  case FCMP_OLT:                                                               \
    *C = isless(A, B);                                                         \
    STR = "FCMP_OLT";                                                          \
    break;                                                                     \
  case FCMP_OLE:                                                               \
    *C = islessequal(A, B);                                                    \
    STR = "FCMP_OLE";                                                          \
    break;                                                                     \
  case FCMP_ONE:                                                               \
    *C = islessgreater(A, B);                                                  \
    STR = "FCMP_ONE";                                                          \
    break;                                                                     \
  case FCMP_ORD:                                                               \
    *C = !isunordered(A, B);                                                   \
    STR = "FCMP_ORD";                                                          \
    break;                                                                     \
  case FCMP_UEQ:                                                               \
    *C = isunordered(A, B);                                                    \
    STR = "FCMP_UEQ";                                                          \
    break;                                                                     \
  case FCMP_UGT:                                                               \
    *C = isunordered(A, B);                                                    \
    STR = "FCMP_UGT";                                                          \
    break;                                                                     \
  case FCMP_UGE:                                                               \
    *C = isunordered(A, B);                                                    \
    STR = "FCMP_UGE";                                                          \
    break;                                                                     \
  case FCMP_ULT:                                                               \
    *C = isunordered(A, B);                                                    \
    str = "FCMP_ULT";                                                          \
    break;                                                                     \
  case FCMP_ULE:                                                               \
    *C = isunordered(A, B);                                                    \
    STR = "FCMP_ULE";                                                          \
    break;                                                                     \
  case FCMP_UNE:                                                               \
    *C = isunordered(A, B);                                                    \
    STR = "FCMP_UNE";                                                          \
    break;                                                                     \
  case FCMP_UNO:                                                               \
    *C = isunordered(A, B);                                                    \
    str = "FCMP_UNO";                                                          \
    break;                                                                     \
  case FCMP_TRUE:                                                              \
    *c = true;                                                                 \
    str = "FCMP_TRUE";                                                         \
    break;                                                                     \
  }

static void _interflop_add_float(const float a, const float b, float *c,
                                 void *context) {
  *c = a + b;
  debug_print_float(context, ARITHMETIC, "+", a, b, *c);
}

static void _interflop_sub_float(const float a, const float b, float *c,
                                 void *context) {
  *c = a - b;
  debug_print_float(context, ARITHMETIC, "-", a, b, *c);
}

static void _interflop_mul_float(const float a, const float b, float *c,
                                 void *context) {
  *c = a * b;
  debug_print_float(context, ARITHMETIC, "*", a, b, *c);
}

static void _interflop_div_float(const float a, const float b, float *c,
                                 void *context) {
  *c = a / b;
  debug_print_float(context, ARITHMETIC, "/", a, b, *c);
}

static void _interflop_cmp_float(const enum FCMP_PREDICATE p, const float a,
                                 const float b, int *c, void *context) {
  char *str = "";
  SELECT_FLOAT_CMP(a, b, c, p, str);
  debug_print_float(context, COMPARISON, str, a, b, *c);
}

static void _interflop_add_double(const double a, const double b, double *c,
                                  void *context) {
  *c = a + b;
  debug_print_double(context, ARITHMETIC, "+", a, b, *c);
}

static void _interflop_sub_double(const double a, const double b, double *c,
                                  void *context) {
  *c = a - b;
  debug_print_double(context, ARITHMETIC, "-", a, b, *c);
}

static void _interflop_mul_double(const double a, const double b, double *c,
                                  void *context) {
  *c = a * b;
  debug_print_double(context, ARITHMETIC, "*", a, b, *c);
}

static void _interflop_div_double(const double a, const double b, double *c,
                                  void *context) {
  *c = a / b;
  debug_print_double(context, ARITHMETIC, "/", a, b, *c);
}

static void _interflop_cmp_double(const enum FCMP_PREDICATE p, const double a,
                                  const double b, int *c, void *context) {
  char *str = "";
  SELECT_FLOAT_CMP(a, b, c, p, str);
  debug_print_double(context, COMPARISON, str, a, b, *c);
}

static struct argp_option options[] = {
    {key_debug_str, KEY_DEBUG, 0, 0, "enable debug output", 0},
    {key_debug_binary_str, KEY_DEBUG_BINARY, 0, 0, "enable binary debug output",
     0},
    {key_no_backend_name_str, KEY_NO_BACKEND_NAME, 0, 0,
     "do not print backend name in debug output", 0},
    {key_print_new_line_str, KEY_PRINT_NEW_LINE, 0, 0,
     "add a new line after debug ouput", 0},
    {key_print_subnormal_normalized_str, KEY_PRINT_SUBNORMAL_NORMALIZED, 0, 0,
     "normalize subnormal numbers", 0},
    {0}};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  t_context *ctx = (t_context *)state->input;
  switch (key) {
  case KEY_DEBUG:
    ctx->debug = true;
    break;
  case KEY_DEBUG_BINARY:
    ctx->debug_binary = true;
    break;
  case KEY_NO_BACKEND_NAME:
    ctx->no_backend_name = true;
    break;
  case KEY_PRINT_NEW_LINE:
    ctx->print_new_line = true;
    break;
  case KEY_PRINT_SUBNORMAL_NORMALIZED:
    ctx->print_subnormal_normalized = true;
    break;
  default:
    return ARGP_ERR_UNKNOWN;
  }

  return 0;
}

void init_context(t_context *context) {
  context->debug = false;
  context->debug_binary = false;
  context->no_backend_name = false;
  context->print_new_line = false;
  context->print_subnormal_normalized = false;
}

static struct argp argp = {options, parse_opt, "", "", NULL, NULL, NULL};

struct interflop_backend_interface_t interflop_init(int argc, char **argv,
                                                    void **context) {

  /* Initialize the logger */
  logger_init();

  t_context *ctx = calloc(1, sizeof(t_context));
  init_context(ctx);
  /* parse backend arguments */
  argp_parse(&argp, argc, argv, 0, 0, ctx);
  *context = ctx;

  /* register %b format */
  register_printf_bit();

  struct interflop_backend_interface_t interflop_backend_ieee = {
      _interflop_add_float,  _interflop_sub_float,  _interflop_mul_float,
      _interflop_div_float,  _interflop_cmp_float,  _interflop_add_double,
      _interflop_sub_double, _interflop_mul_double, _interflop_div_double,
      _interflop_cmp_double};

  return interflop_backend_ieee;
}
