#include <argp.h>
#include <stdio.h>
#include <stdlib.h>

#include "../../common/interflop.h"

typedef struct {
  int debug;
} t_context;

#define debug_print(fmt, ...)                                                  \
  do {                                                                         \
    if (((t_context *)context)->debug)                                         \
      fprintf(stderr, fmt, __VA_ARGS__);                                       \
  } while (0)

static void _interflop_add_float(float a, float b, float *c, void *context) {
  *c = a + b;
  debug_print("interflop_ieee %g + %g -> %g\n", a, b, *c);
}

static void _interflop_sub_float(float a, float b, float *c, void *context) {
  *c = a - b;
  debug_print("interflop_ieee %g - %g -> %g\n", a, b, *c);
}

static void _interflop_mul_float(float a, float b, float *c, void *context) {
  *c = a * b;
  debug_print("interflop_ieee %g * %g -> %g\n", a, b, *c);
}

static void _interflop_div_float(float a, float b, float *c, void *context) {
  *c = a / b;
  debug_print("interflop_ieee %g / %g -> %g\n", a, b, *c);
}

static void _interflop_cmp_float(enum FCMP_PREDICATE p, float a, float b,
                                 int *c, void *context) {
  switch (p) {
  case FCMP_FALSE:
    *c = 0;
    break;
  case FCMP_OEQ:
    *c = ((!isnan(a)) && (!isnan(b)) && (a == b));
    break;
  case FCMP_OGT:
    *c = ((!isnan(a)) && (!isnan(b)) && (a > b));
    break;
  case FCMP_OGE:
    *c = ((!isnan(a)) && (!isnan(b)) && (a >= b));
    break;
  case FCMP_OLT:
    *c = ((!isnan(a)) && (!isnan(b)) && (a < b));
    break;
  case FCMP_OLE:
    *c = ((!isnan(a)) && (!isnan(b)) && (a <= b));
    break;
  case FCMP_ONE:
    *c = ((!isnan(a)) && (!isnan(b)) && (a != b));
    break;
  case FCMP_ORD:
    *c = ((!isnan(a)) && (!isnan(b)));
    break;
  case FCMP_UEQ:
    *c = ((!isnan(a)) || (!isnan(b)) || (a == b));
    break;
  case FCMP_UGT:
    *c = ((!isnan(a)) || (!isnan(b)) || (a > b));
    break;
  case FCMP_UGE:
    *c = ((!isnan(a)) || (!isnan(b)) || (a >= b));
    break;
  case FCMP_ULT:
    *c = ((!isnan(a)) || (!isnan(b)) || (a < b));
    break;
  case FCMP_ULE:
    *c = ((!isnan(a)) || (!isnan(b)) || (a <= b));
    break;
  case FCMP_UNE:
    *c = ((!isnan(a)) || (!isnan(b)) || (a != b));
    break;
  case FCMP_UNO:
    *c = ((!isnan(a)) || (!isnan(b)));
    break;
  case FCMP_TRUE:
    *c = 1;
    break;
  }
  debug_print("interflop_ieee %g FCMP(%d) %g -> %s\n", a, p, b,
              *c ? "true" : "false");
}

static void _interflop_add_double(double a, double b, double *c,
                                  void *context) {
  *c = a + b;
  debug_print("interflop_ieee %g + %g -> %g\n", a, b, *c);
}

static void _interflop_sub_double(double a, double b, double *c,
                                  void *context) {
  *c = a - b;
  debug_print("interflop_ieee %g - %g -> %g\n", a, b, *c);
}

static void _interflop_mul_double(double a, double b, double *c,
                                  void *context) {
  *c = a * b;
  debug_print("interflop_ieee %g * %g -> %g\n", a, b, *c);
}

static void _interflop_div_double(double a, double b, double *c,
                                  void *context) {
  *c = a / b;
  debug_print("interflop_ieee %g / %g -> %g\n", a, b, *c);
}

static void _interflop_cmp_double(enum FCMP_PREDICATE p, double a, double b,
                                  int *c, void *context) {
  switch (p) {
  case FCMP_FALSE:
    *c = 0;
    break;
  case FCMP_OEQ:
    *c = ((!isnan(a)) && (!isnan(b)) && (a == b));
    break;
  case FCMP_OGT:
    *c = ((!isnan(a)) && (!isnan(b)) && (a > b));
    break;
  case FCMP_OGE:
    *c = ((!isnan(a)) && (!isnan(b)) && (a >= b));
    break;
  case FCMP_OLT:
    *c = ((!isnan(a)) && (!isnan(b)) && (a < b));
    break;
  case FCMP_OLE:
    *c = ((!isnan(a)) && (!isnan(b)) && (a <= b));
    break;
  case FCMP_ONE:
    *c = ((!isnan(a)) && (!isnan(b)) && (a != b));
    break;
  case FCMP_ORD:
    *c = ((!isnan(a)) && (!isnan(b)));
    break;
  case FCMP_UEQ:
    *c = ((!isnan(a)) || (!isnan(b)) || (a == b));
    break;
  case FCMP_UGT:
    *c = ((!isnan(a)) || (!isnan(b)) || (a > b));
    break;
  case FCMP_UGE:
    *c = ((!isnan(a)) || (!isnan(b)) || (a >= b));
    break;
  case FCMP_ULT:
    *c = ((!isnan(a)) || (!isnan(b)) || (a < b));
    break;
  case FCMP_ULE:
    *c = ((!isnan(a)) || (!isnan(b)) || (a <= b));
    break;
  case FCMP_UNE:
    *c = ((!isnan(a)) || (!isnan(b)) || (a != b));
    break;
  case FCMP_UNO:
    *c = ((!isnan(a)) || (!isnan(b)));
    break;
  case FCMP_TRUE:
    *c = 1;
    break;
  }
  debug_print("interflop_ieee %g FCMP(%d) %g -> %s\n", a, p, b,
              *c ? "true" : "false");
}

static struct argp_option options[] = {
  /* --debug, sets the variable debug = true */
  {"debug", 'd', 0, 0, "enable debug output"},
  {0}};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  t_context * ctx = (t_context*) state->input;
  switch (key)
    {
    case 'd':
      ctx->debug = 1;
      break;
    default:
      return ARGP_ERR_UNKNOWN;
    }
}

static struct argp argp = {options, parse_opt, "", ""};

struct interflop_backend_interface_t interflop_init(int argc, char **argv,
                                                    void **context) {
  t_context * ctx = malloc(sizeof(t_context));
  /* parse backend arguments */
  argp_parse (&argp, argc, argv, 0, 0, ctx);
  *context = ctx;

  struct interflop_backend_interface_t interflop_backend_ieee = {
      _interflop_add_float,  _interflop_sub_float,  _interflop_mul_float,
      _interflop_div_float,  _interflop_cmp_float,  _interflop_add_double,
      _interflop_sub_double, _interflop_mul_double, _interflop_div_double,
      _interflop_cmp_double};

  return interflop_backend_ieee;
}
