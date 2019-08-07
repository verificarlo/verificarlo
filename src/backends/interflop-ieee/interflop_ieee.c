#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "../../common/interflop.h"

static bool debug = false;
#define debug_print(fmt, ...) \
            do { if (debug) fprintf(stderr, fmt, __VA_ARGS__); } while (0)


static void _interflop_add_float(float a, float b , float* c, void* context) {
  *c = a + b;
  debug_print("interflop %g + %g -> %g\n", a, b, *c);
}

static void _interflop_sub_float(float a, float b, float* c, void* context) {
  *c = a - b;
  debug_print("interflop %g - %g -> %g\n", a, b, *c);
}

static void _interflop_mul_float(float a, float b, float* c, void* context) {
  *c = a * b;
  debug_print("interflop %g * %g -> %g\n", a, b, *c);
}

static void _interflop_div_float(float a, float b, float* c, void* context) {
  *c = a / b;
  debug_print("interflop %g / %g -> %g\n", a, b, *c);
}

static void _interflop_cmp_float(enum FCMP_PREDICATE p, float a, float b, bool* c, void* context) {
  switch(p) {
    case FCMP_FALSE: *c = false;
    case FCMP_OEQ: *c = ((!isnan(a))&&(!isnan(b))&&(a==b));
    case FCMP_OGT: *c = ((!isnan(a))&&(!isnan(b))&&(a>b));
    case FCMP_OGE: *c = ((!isnan(a))&&(!isnan(b))&&(a>=b));
    case FCMP_OLT: *c = ((!isnan(a))&&(!isnan(b))&&(a<b));
    case FCMP_OLE: *c = ((!isnan(a))&&(!isnan(b))&&(a<=b));
    case FCMP_ONE: *c = ((!isnan(a))&&(!isnan(b))&&(a!=b));
    case FCMP_ORD: *c = ((!isnan(a))&&(!isnan(b)));
    case FCMP_UEQ: *c = ((!isnan(a))||(!isnan(b))||(a==b));
    case FCMP_UGT: *c = ((!isnan(a))||(!isnan(b))||(a>b));
    case FCMP_UGE: *c = ((!isnan(a))||(!isnan(b))||(a>=b));
    case FCMP_ULT: *c = ((!isnan(a))||(!isnan(b))||(a<b));
    case FCMP_ULE: *c = ((!isnan(a))||(!isnan(b))||(a<=b));
    case FCMP_UNE: *c = ((!isnan(a))||(!isnan(b))||(a!=b));
    case FCMP_UNO: *c = ((!isnan(a))||(!isnan(b)));
    case FCMP_TRUE: *c = true;
  }
  debug_print("interflop %g FCMP(%d) %g -> %g\n", a, p, b, *c);
}

static void _interflop_add_double(double a, double b , double* c, void* context) {
  *c = a + b;
  debug_print("interflop %g + %g -> %g\n", a, b, *c);
}

static void _interflop_sub_double(double a, double b, double* c, void* context) {
  *c = a - b;
  debug_print("interflop %g - %g -> %g\n", a, b, *c);
}

static void _interflop_mul_double(double a, double b, double* c, void* context) {
  *c = a * b;
  debug_print("interflop %g * %g -> %g\n", a, b, *c);
}

static void _interflop_div_double(double a, double b, double* c, void* context) {
  *c = a / b;
  debug_print("interflop %g / %g -> %g\n", a, b, *c);
}

static void _interflop_cmp_double(enum FCMP_PREDICATE p, double a, double b, bool* c, void *context) {
  switch(p) {
    case FCMP_FALSE: *c = false;
    case FCMP_OEQ: *c = ((!isnan(a))&&(!isnan(b))&&(a==b));
    case FCMP_OGT: *c = ((!isnan(a))&&(!isnan(b))&&(a>b));
    case FCMP_OGE: *c = ((!isnan(a))&&(!isnan(b))&&(a>=b));
    case FCMP_OLT: *c = ((!isnan(a))&&(!isnan(b))&&(a<b));
    case FCMP_OLE: *c = ((!isnan(a))&&(!isnan(b))&&(a<=b));
    case FCMP_ONE: *c = ((!isnan(a))&&(!isnan(b))&&(a!=b));
    case FCMP_ORD: *c = ((!isnan(a))&&(!isnan(b)));
    case FCMP_UEQ: *c = ((!isnan(a))||(!isnan(b))||(a==b));
    case FCMP_UGT: *c = ((!isnan(a))||(!isnan(b))||(a>b));
    case FCMP_UGE: *c = ((!isnan(a))||(!isnan(b))||(a>=b));
    case FCMP_ULT: *c = ((!isnan(a))||(!isnan(b))||(a<b));
    case FCMP_ULE: *c = ((!isnan(a))||(!isnan(b))||(a<=b));
    case FCMP_UNE: *c = ((!isnan(a))||(!isnan(b))||(a!=b));
    case FCMP_UNO: *c = ((!isnan(a))||(!isnan(b)));
    case FCMP_TRUE: *c = true;
  }
  debug_print("interflop %g FCMP(%d) %g -> %g\n", a, p, b, *c);
}

struct interflop_backend_interface_t interflop_init(void ** context) {

  char * interflop_backend_debug = getenv("INTERFLOP_DEBUG");
  if (interflop_backend_debug != NULL) {
    debug = true;
  }

  struct interflop_backend_interface_t interflop_backend_ieee = {
    _interflop_add_float,
    _interflop_sub_float,
    _interflop_mul_float,
    _interflop_div_float,
    _interflop_cmp_float,
    _interflop_add_double,
    _interflop_sub_double,
    _interflop_mul_double,
    _interflop_div_double,
    _interflop_cmp_double
  };

  return interflop_backend_ieee;
}
