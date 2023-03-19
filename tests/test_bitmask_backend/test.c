#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "interflop/common/float_const.h"

typedef union {
  double f64;
  int64_t s64;
} binary64;

typedef union {
  float f32;
  int32_t s32;
} binary32;

#define OPERATOR(a, b)                                                         \
  switch (op) {                                                                \
  case '+':                                                                    \
    return a + b;                                                              \
  case '-':                                                                    \
    return a - b;                                                              \
  case 'x':                                                                    \
    return a * b;                                                              \
  case '/':                                                                    \
    return a / b;                                                              \
  default:                                                                     \
    fprintf(stderr, "Error: unknown operation\n");                             \
    abort();                                                                   \
  }

REAL operator(char op, REAL a, REAL b) { OPERATOR(a, b) }

double get_rand_double() {
  binary64 b64 = {.s64 = mrand48() % DOUBLE_PLUS_INF};
  return b64.f64;
}

float get_rand_float() {
  binary32 b32 = {.s32 = rand() % FLOAT_PLUS_INF};
  return b32.f32;
}

#define GET_RAND(X)                                                            \
  _Generic((X), double : get_rand_double, float : get_rand_float)

REAL get_rand() {
  typeof(REAL) x;
  return GET_RAND(x)();
}

static void do_test(const char op, REAL a, REAL b) {
  for (int i = 0; i < SAMPLES; i++) {
    printf("%.13a\n", operator(op, a, b));
  }
}

int main(int argc, const char *argv[]) {
  /* initialize random seed */
  srand(0);
  srand48(0);

  if (argc != 2) {
    fprintf(stderr, "usage: ./test <op>\n");
    exit(1);
  }
  const char op = argv[1][0];

  do_test(op, get_rand(), get_rand());

  return EXIT_SUCCESS;
}
