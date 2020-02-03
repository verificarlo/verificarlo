#include <argp.h>
#include <ieee754.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../common/interflop.h"

typedef struct {
  int debug;
  int debug_binary;
} t_context;

#define STRING_MAX 256

void double_to_binary(double d, char *s_val) {
  union ieee754_double ud;
  unsigned char sign_field;
  unsigned short exponent_field;
  short exponent;
  unsigned long long fraction_field, significand;
  int i, str_i, start = 0, end = 52;
  char s_exp[80];

  ud.d = d;
  sign_field = ud.ieee.negative;
  exponent_field = ud.ieee.exponent;
  fraction_field = ud.ieee.mantissa0;
  fraction_field = fraction_field << 32;
  fraction_field |= ud.ieee.mantissa1;
  str_i = 0;

  // Print a minus sign, if necessary
  if (sign_field == 1)
    s_val[str_i++] = '-';

  if (exponent_field == 0 && fraction_field == 0) {
    s_val[str_i++] = '0'; // Number is zero
    s_val[str_i++] = 0;   // NULL Terminator
  } else {
    if (exponent_field == 0 && fraction_field != 0) { // Subnormal number
      significand = fraction_field;                   // No implicit 1 bit
      exponent = -(IEEE754_DOUBLE_BIAS - 1); // Exponents decrease from here
      while (((significand >> (52 - start)) & 1) == 0) {
        exponent--;
        start++;
      }
    } else { // Normalized number (ignoring INFs, NANs)
      significand = fraction_field | (1ULL << 52);     // Implicit 1 bit
      exponent = exponent_field - IEEE754_DOUBLE_BIAS; // Subtract bias
    }

    // Suppress trailing 0s
    while (((significand >> (52 - end)) & 1) == 0)
      end--;

    // Print the significant bits
    for (i = start; i <= end; i++) {
      if (((significand >> (52 - i)) & 1) == 1)
        s_val[str_i++] = '1';
      else
        s_val[str_i++] = '0';
      if (i == start)
        s_val[str_i++] = '.';
    }

    if (start == end) // d is power of 2
      s_val[str_i++] = '0';

    s_val[str_i++] = 0; // NULL Terminator

    // Exponent
    sprintf(s_exp, " x 2^%d", exponent);
    strcat(s_val, s_exp);
  }
}

void inline debug_print_flt(void *context, int typeop, char *op, double a,
                            double b, double c) {
  if (((t_context *)context)->debug) {
    if (typeop == 0)
      fprintf(stderr, "interflop_ieee %g %s %g -> %g\n", a, op, b, c);
    else
      fprintf(stderr, "interflop_ieee %g %s %g -> %s\n", a, op, b,
              c ? "true" : "false");
  }
  if (((t_context *)context)->debug_binary) {
    char f_str[STRING_MAX];
    if (typeop == 0) {
      fprintf(stderr, "interflop_ieee_bin\n");
      double_to_binary(a, f_str);
      fprintf(stderr, "%s %s \n", f_str, op);
      double_to_binary(b, f_str);
      fprintf(stderr, "%s -> \n", f_str);
      double_to_binary(c, f_str);
      fprintf(stderr, "%s \n\n", f_str);
    } else {
      fprintf(stderr, "interflop_ieee_bin\n");
      double_to_binary(a, f_str);
      fprintf(stderr, "%s [%s] \n", f_str, op);
      double_to_binary(b, f_str);
      fprintf(stderr, "%s -> %s \n\n", f_str, c ? "true" : "false");
    }
  }
}

static void _interflop_add_float(float a, float b, float *c, void *context) {
  *c = a + b;
  debug_print_flt(context, 0, "+", (double)a, (double)b, (double)*c);
}

static void _interflop_sub_float(float a, float b, float *c, void *context) {
  *c = a - b;
  debug_print_flt(context, 0, "-", (double)a, (double)b, (double)*c);
}

static void _interflop_mul_float(float a, float b, float *c, void *context) {
  *c = a * b;
  debug_print_flt(context, 0, "*", (double)a, (double)b, (double)*c);
}

static void _interflop_div_float(float a, float b, float *c, void *context) {
  *c = a / b;
  debug_print_flt(context, 0, "/", (double)a, (double)b, (double)*c);
}

static void _interflop_cmp_float(enum FCMP_PREDICATE p, float a, float b,
                                 int *c, void *context) {
  char *str;

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
  debug_print_flt(context, 1, str, (double)a, (double)b, (double)*c);
}

static void _interflop_add_double(double a, double b, double *c,
                                  void *context) {
  *c = a + b;
  debug_print_flt(context, 0, "+", a, b, *c);
}

static void _interflop_sub_double(double a, double b, double *c,
                                  void *context) {
  *c = a - b;
  debug_print_flt(context, 0, "-", a, b, *c);
}

static void _interflop_mul_double(double a, double b, double *c,
                                  void *context) {
  *c = a * b;
  debug_print_flt(context, 0, "*", a, b, *c);
}

static void _interflop_div_double(double a, double b, double *c,
                                  void *context) {
  *c = a / b;
  debug_print_flt(context, 0, "/", a, b, *c);
}

static void _interflop_cmp_double(enum FCMP_PREDICATE p, double a, double b,
                                  int *c, void *context) {
  char *str;

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
  debug_print_flt(context, 1, str, a, b, *c);
}

static struct argp_option options[] = {
    /* --debug, sets the variable debug = true */
    {"debug", 'd', 0, 0, "enable debug output"},
    {"debug_binary", 'b', 0, 0, "enable binary debug output"},
    {0}};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  t_context *ctx = (t_context *)state->input;
  switch (key) {
  case 'd':
    ctx->debug = 1;
    break;
  case 'b':
    ctx->debug_binary = 1;
    break;
  default:
    return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

static struct argp argp = {options, parse_opt, "", ""};

struct interflop_backend_interface_t interflop_init(int argc, char **argv,
                                                    void **context) {
  t_context *ctx = calloc(1, sizeof(t_context));
  /* parse backend arguments */
  argp_parse(&argp, argc, argv, 0, 0, ctx);
  *context = ctx;

  struct interflop_backend_interface_t interflop_backend_ieee = {
      _interflop_add_float,  _interflop_sub_float,  _interflop_mul_float,
      _interflop_div_float,  _interflop_cmp_float,  _interflop_add_double,
      _interflop_sub_double, _interflop_mul_double, _interflop_div_double,
      _interflop_cmp_double};

  return interflop_backend_ieee;
}
