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
#include "printf_specifier.h"

typedef struct {
  bool debug;
  bool debug_binary;
  bool no_print_debug_mode;
  bool print_new_line;
} t_context;

typedef enum {
  ARITHMETIC = 0,
  COMPARISON = 1,
} operation_type;

#define STRING_MAX 256

#define FMT(X) _Generic(X, float : "b", double : "lb")

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

#define DEBUG_HEADER "interflop_ieee "
#define DEBUG_BINARY_HEADER "interflop_ieee_bin "

/* This macro print the debug information for a, b and c */
/* the debug_print function handles automatically the format (decimal or binary) */
/* depending on the context */
#define DEBUG_PRINT(context, typeop, op, a, b, c)                              \
  {                                                                            \
    bool debug = ((t_context *)context)->debug ? true : false;                 \
    bool debug_binary = ((t_context *)context)->debug_binary ? true : false;   \
    if (debug || debug_binary) {                                               \
      bool print_header =                                                      \
          (((t_context *)context)->no_print_debug_mode) ? false : true;        \
      char *header = (debug) ? DEBUG_HEADER : DEBUG_BINARY_HEADER;             \
      char *float_fmt = FMT(a);                                                \
      if (print_header)                                                        \
        debug_print(context, float_fmt, header);                               \
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

void inline debug_print_float(void *context, operation_type typeop,
                              const char *op, float a, float b, float c) {
  DEBUG_PRINT(context, typeop, op, a, b, c);
}

void inline debug_print_double(void *context, operation_type typeop,
                               const char *op, double a, double b, double c) {
  DEBUG_PRINT(context, typeop, op, a, b, c);
}

static void _interflop_add_float(float a, float b, float *c, void *context) {
  *c = a + b;
  debug_print_float(context, ARITHMETIC, "+", a, b, *c);
}

static void _interflop_sub_float(float a, float b, float *c, void *context) {
  *c = a - b;
  debug_print_float(context, ARITHMETIC, "-", a, b, *c);
}

static void _interflop_mul_float(float a, float b, float *c, void *context) {
  *c = a * b;
  debug_print_float(context, ARITHMETIC, "*", a, b, *c);
}

static void _interflop_div_float(float a, float b, float *c, void *context) {
  *c = a / b;
  debug_print_float(context, ARITHMETIC, "/", a, b, *c);
}

static void _interflop_cmp_float(enum FCMP_PREDICATE p, float a, float b,
                                 int *c, void *context) {
  char *str = "";

  switch (p) {
  case FCMP_FALSE:
    *c = 0;
    str = "FCMP_FALSE";
    break;
  case FCMP_OEQ:
    *c = ((!isnan(a)) && (!isnan(b)) && (a == b));
    str = "FCMP_OEQ";
    break;
  case FCMP_OGT:
    *c = ((!isnan(a)) && (!isnan(b)) && (a > b));
    str = "FCMP_OGT";
    break;
  case FCMP_OGE:
    *c = ((!isnan(a)) && (!isnan(b)) && (a >= b));
    str = "FCMP_OGE";
    break;
  case FCMP_OLT:
    *c = ((!isnan(a)) && (!isnan(b)) && (a < b));
    str = "FCMP_OLT";
    break;
  case FCMP_OLE:
    *c = ((!isnan(a)) && (!isnan(b)) && (a <= b));
    str = "FCMP_OLE";
    break;
  case FCMP_ONE:
    *c = ((!isnan(a)) && (!isnan(b)) && (a != b));
    str = "FCMP_ONE";
    break;
  case FCMP_ORD:
    *c = ((!isnan(a)) && (!isnan(b)));
    str = "FCMP_ORD";
    break;
  case FCMP_UEQ:
    *c = ((!isnan(a)) || (!isnan(b)) || (a == b));
    str = "FCMP_UEQ";
    break;
  case FCMP_UGT:
    *c = ((!isnan(a)) || (!isnan(b)) || (a > b));
    str = "";
    break;
  case FCMP_UGE:
    *c = ((!isnan(a)) || (!isnan(b)) || (a >= b));
    str = "FCMP_UGT";
    break;
  case FCMP_ULT:
    *c = ((!isnan(a)) || (!isnan(b)) || (a < b));
    str = "FCMP_ULT";
    break;
  case FCMP_ULE:
    *c = ((!isnan(a)) || (!isnan(b)) || (a <= b));
    str = "FCMP_ULE";
    break;
  case FCMP_UNE:
    *c = ((!isnan(a)) || (!isnan(b)) || (a != b));
    str = "FCMP_UNE";
    break;
  case FCMP_UNO:
    *c = ((!isnan(a)) || (!isnan(b)));
    str = "FCMP_UNO";
    break;
  case FCMP_TRUE:
    *c = 1;
    str = "FCMP_TRUE";
    break;
  }
  debug_print_float(context, COMPARISON, str, a, b, *c);
}

static void _interflop_add_double(double a, double b, double *c,
                                  void *context) {
  *c = a + b;
  debug_print_double(context, ARITHMETIC, "+", a, b, *c);
}

static void _interflop_sub_double(double a, double b, double *c,
                                  void *context) {
  *c = a - b;
  debug_print_double(context, ARITHMETIC, "-", a, b, *c);
}

static void _interflop_mul_double(double a, double b, double *c,
                                  void *context) {
  *c = a * b;
  debug_print_double(context, ARITHMETIC, "*", a, b, *c);
}

static void _interflop_div_double(double a, double b, double *c,
                                  void *context) {
  *c = a / b;
  debug_print_double(context, ARITHMETIC, "/", a, b, *c);
}

static void _interflop_cmp_double(enum FCMP_PREDICATE p, double a, double b,
                                  int *c, void *context) {
  char *str = "";

  switch (p) {
  case FCMP_FALSE:
    *c = 0;
    str = "FCMP_FALSE";
    break;
  case FCMP_OEQ:
    *c = ((!isnan(a)) && (!isnan(b)) && (a == b));
    str = "FCMP_OEQ";
    break;
  case FCMP_OGT:
    *c = ((!isnan(a)) && (!isnan(b)) && (a > b));
    str = "FCMP_OGT";
    break;
  case FCMP_OGE:
    *c = ((!isnan(a)) && (!isnan(b)) && (a >= b));
    str = "FCMP_OGE";
    break;
  case FCMP_OLT:
    *c = ((!isnan(a)) && (!isnan(b)) && (a < b));
    str = "FCMP_OLT";
    break;
  case FCMP_OLE:
    *c = ((!isnan(a)) && (!isnan(b)) && (a <= b));
    str = "FCMP_OLE";
    break;
  case FCMP_ONE:
    *c = ((!isnan(a)) && (!isnan(b)) && (a != b));
    str = "FCMP_ONE";
    break;
  case FCMP_ORD:
    *c = ((!isnan(a)) && (!isnan(b)));
    str = "FCMP_ORD";
    break;
  case FCMP_UEQ:
    *c = ((!isnan(a)) || (!isnan(b)) || (a == b));
    str = "FCMP_UEQ";
    break;
  case FCMP_UGT:
    *c = ((!isnan(a)) || (!isnan(b)) || (a > b));
    str = "";
    break;
  case FCMP_UGE:
    *c = ((!isnan(a)) || (!isnan(b)) || (a >= b));
    str = "FCMP_UGT";
    break;
  case FCMP_ULT:
    *c = ((!isnan(a)) || (!isnan(b)) || (a < b));
    str = "FCMP_ULT";
    break;
  case FCMP_ULE:
    *c = ((!isnan(a)) || (!isnan(b)) || (a <= b));
    str = "FCMP_ULE";
    break;
  case FCMP_UNE:
    *c = ((!isnan(a)) || (!isnan(b)) || (a != b));
    str = "FCMP_UNE";
    break;
  case FCMP_UNO:
    *c = ((!isnan(a)) || (!isnan(b)));
    str = "FCMP_UNO";
    break;
  case FCMP_TRUE:
    *c = 1;
    str = "FCMP_TRUE";
    break;
  }
  debug_print_double(context, COMPARISON, str, a, b, *c);
}

static struct argp_option options[] = {
    /* --debug, sets the variable debug = true */
    {"debug", 'd', 0, 0, "enable debug output"},
    {"debug-binary", 'b', 0, 0, "enable binary debug output"},
    {"no-print-debug-mode", 's', 0, 0,
     "do not print debug mode before debug outputting"},
    {"print-new-line", 'n', 0, 0, "print new lines after debug output"},
    {0}};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  t_context *ctx = (t_context *)state->input;
  switch (key) {
  case 'd':
    ctx->debug = true;
    break;
  case 'b':
    ctx->debug_binary = true;
    break;
  case 's':
    ctx->no_print_debug_mode = true;
    break;
  case 'n':
    ctx->print_new_line = true;
    break;
  default:
    return ARGP_ERR_UNKNOWN;
  }

  return 0;
}

void init_context(t_context *context) {
  context->debug = false;
  context->debug_binary = false;
  context->no_print_debug_mode = false;
  context->print_new_line = false;
}

static struct argp argp = {options, parse_opt, "", ""};

struct interflop_backend_interface_t interflop_init(int argc, char **argv,
                                                    void **context) {
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
