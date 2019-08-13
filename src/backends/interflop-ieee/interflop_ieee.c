#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include "../../common/interflop.h"

static int debug = 0;
#define debug_print(fmt, ...)                                                  \
  do {                                                                         \
    if (debug)                                                                 \
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

struct interflop_backend_interface_t interflop_init(int argc, char **argv,
                                                    void **context) {
  /* parse backend arguments */
  while (1) {
    static struct option long_options[] = {
        /* --debug, sets the variable debug = true */
        {"debug", no_argument, &debug, 1},
        {0, 0, 0, 0}};
    int option_index = 0;
    int c = getopt_long_only(argc, argv, "", long_options, &option_index);
    /* Detect end of the options or errors */
    if (c == -1)
      break;
    if (c != 0)
      abort();
  }
  /* since backends can be loaded multiple twice, reset getopt state */
  optind = 1;

  struct interflop_backend_interface_t interflop_backend_ieee = {
      _interflop_add_float,  _interflop_sub_float,  _interflop_mul_float,
      _interflop_div_float,  _interflop_cmp_float,  _interflop_add_double,
      _interflop_sub_double, _interflop_mul_double, _interflop_div_double,
      _interflop_cmp_double};

  return interflop_backend_ieee;
}
