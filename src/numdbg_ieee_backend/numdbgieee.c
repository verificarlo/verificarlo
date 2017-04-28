#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "../vfcwrapper/vfcwrapper.h"

static bool debug = false;
#define debug_print(fmt, ...) \
            do { if (debug) fprintf(stderr, fmt, __VA_ARGS__); } while (0)


static void _numdbg_add_float(float a, float b , float* c, void* context) {
  *c = a + b;
  debug_print("numdbg %g + %g -> %g\n", a, b, c);
}

static void _numdbg_sub_float(float a, float b, float* c, void* context) {
  *c = a - b;
  debug_print("numdbg %g - %g -> %g\n", a, b, c);
}

static void _numdbg_mul_float(float a, float b, float* c, void* context) {
  *c = a * b;
  debug_print("numdbg %g * %g -> %g\n", a, b, c);
}

static void _numdbg_div_float(float a, float b, float* c, void* context) {
  *c = a / b;
  debug_print("numdbg %g / %g -> %g\n", a, b, c);
}

static void _numdbg_add_double(double a, double b , double* c, void* context) {
  *c = a + b;
  debug_print("numdbg %g + %g -> %g\n", a, b, c);
}

static void _numdbg_sub_double(double a, double b, double* c, void* context) {
  *c = a - b;
  debug_print("numdbg %g - %g -> %g\n", a, b, c);
}

static void _numdbg_mul_double(double a, double b, double* c, void* context) {
  *c = a * b;
  debug_print("numdbg %g * %g -> %g\n", a, b, c);
}

static void _numdbg_div_double(double a, double b, double* c, void* context) {
  *c = a / b;
  debug_print("numdbg %g / %g -> %g\n", a, b, c);
}

struct numdbg_backend_interface_t numdbg_init(void ** context) {

  char * numdbg_backend_debug = getenv("NUMDBG_BACKEND_DEBUG");
  if (numdbg_backend_debug != NULL) {
    debug = true;
  }

  struct numdbg_backend_interface_t numdbg_backend_ieee = {
    _numdbg_add_float,
    _numdbg_sub_float,
    _numdbg_mul_float,
    _numdbg_div_float,
    _numdbg_add_double,
    _numdbg_sub_double,
    _numdbg_mul_double,
    _numdbg_div_double
  };

  return numdbg_backend_ieee;
}
