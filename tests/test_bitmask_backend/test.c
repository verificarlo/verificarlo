#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "../../src/common/float_const.h"

typedef union {
  double f64;
  int64_t s64;
} binary64;

typedef union {
  float f32;
  int32_t s32;
} binary32;

double get_rand_double() {
  binary64 b64 = {.s64 = mrand48() % DOUBLE_PLUS_INF };
  return b64.f64;
}

float get_rand_float() {
  binary32 b32 = {.s32 = rand() % FLOAT_PLUS_INF };
  return b32.f32;
}

#define GET_RAND(X)                                                            \
  _Generic((X), double : get_rand_double, float : get_rand_float)

REAL operate(REAL a, REAL b) { return a OPERATION b; }

REAL get_rand() {
  typeof(REAL) x;
  return GET_RAND(x)();
}

static void do_test(REAL a, REAL b) {
  for (int i = 0; i < SAMPLES; i++) {
    printf("%.13a\n", operate(a,b));
  }
}

int main(void) {
  /* initialize random seed */
  srand(0);
  srand48(0);

  do_test(get_rand(), get_rand());

  return EXIT_SUCCESS;
}
