#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "../vfcwrapper/vfcwrapper.h"

static bool debug = false;
#define debug_print(fmt, ...) \
            do { if (debug) fprintf(stderr, fmt, __VA_ARGS__); } while (0)


static void _interflop_add_float(float a, float b , float* c, void* context) {
  *c = a + b;
  debug_print("interflop %g + %g -> %g\n", a, b, c);
}

static void _interflop_sub_float(float a, float b, float* c, void* context) {
  *c = a - b;
  debug_print("interflop %g - %g -> %g\n", a, b, c);
}

static void _interflop_mul_float(float a, float b, float* c, void* context) {
  *c = a * b;
  debug_print("interflop %g * %g -> %g\n", a, b, c);
}

static void _interflop_div_float(float a, float b, float* c, void* context) {
  *c = a / b;
  debug_print("interflop %g / %g -> %g\n", a, b, c);
}

static void _interflop_add_double(double a, double b , double* c, void* context) {
  *c = a + b;
  debug_print("interflop %g + %g -> %g\n", a, b, c);
}

static void _interflop_sub_double(double a, double b, double* c, void* context) {
  *c = a - b;
  debug_print("interflop %g - %g -> %g\n", a, b, c);
}

static void _interflop_mul_double(double a, double b, double* c, void* context) {
  *c = a * b;
  debug_print("interflop %g * %g -> %g\n", a, b, c);
}

static void _interflop_div_double(double a, double b, double* c, void* context) {
  *c = a / b;
  debug_print("interflop %g / %g -> %g\n", a, b, c);
}

struct interflop_backend_interface_t interflop_init(void ** context) {

  char * interflop_backend_debug = getenv("NUMDBG_BACKEND_DEBUG");
  if (interflop_backend_debug != NULL) {
    debug = true;
  }

  struct interflop_backend_interface_t interflop_backend_ieee = {
    _interflop_add_float,
    _interflop_sub_float,
    _interflop_mul_float,
    _interflop_div_float,
    _interflop_add_double,
    _interflop_sub_double,
    _interflop_mul_double,
    _interflop_div_double
  };

  return interflop_backend_ieee;
}
