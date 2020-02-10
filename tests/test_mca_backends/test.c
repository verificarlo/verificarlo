#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "../../src/common/float_const.h"

#ifndef NB_ITER
#define NB_ITER 50
#endif

typedef union {
  double f64;
  int64_t s64;
} binary64;

typedef union {
  float f32;
  int32_t s32;
} binary32;

double get_rand_double() {
  binary64 b64 = { .s64 = mrand48() % DOUBLE_PLUS_INF };
  return b64.f64;
}

float get_rand_float() {
  binary32 b32 = { .s32 = rand() % FLOAT_PLUS_INF };
  return b32.f32;
}

#define FMT(X) _Generic((X), double : "%.13a", float: "%.6a")
#define GET_RAND(X) _Generic((X), double: get_rand_double, float: get_rand_float)

REAL operate(REAL a, REAL b) {
  return a OPERATION b;
}

REAL get_rand() {
  typeof(REAL) x;
  return GET_RAND(x)();
}

#define QUOTE(name) #name
#define STR(macro) QUOTE(macro)
#define STR_OPERATION STR(OPERATION)

static void do_test(char *fmt, REAL a, REAL b) {
  int i;
  for (i = 0; i < SAMPLES; i++) {
    printf(fmt, a, b, operate(a,b));
  }
}

int main(void) {
  /* initialize random seed */
  srand(0);
  srand48(0);

  REAL x;
  char *flt_fmt = FMT(x);
  char fmt[1024];
  sprintf(fmt, "%s %s %s = %s\n", flt_fmt, STR_OPERATION, flt_fmt, flt_fmt);
  /* Test with NB_ITER different random operands */
  int i;
  for (i=0; i<NB_ITER; i ++) {
    do_test(fmt, get_rand(), get_rand());
  }

  return 0;
}
