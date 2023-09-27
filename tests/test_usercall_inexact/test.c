#include <stdio.h>
#include <stdlib.h>

#include <interflop/interflop.h>

#ifndef REAL
#error "REAL type not defined"
#endif

#ifndef N
#error "Number of repetitions not defined"
#endif

void binary32_test(int p) {
  float x = 0.1f;
  fprintf(stderr, "Before %.6a\n", x);
  interflop_call(INTERFLOP_INEXACT_ID, FFLOAT, &x, p);
  fprintf(stderr, "After  %.6a\n", x);
}

void binary64_test(int p) {
  double x = 0.1;
  fprintf(stderr, "Before %.13a\n", x);
  interflop_call(INTERFLOP_INEXACT_ID, FDOUBLE, &x, p);
  fprintf(stderr, "After  %.13a\n", x);
}

#define DO_TEST(X, P)                                                          \
  _Generic(X, float : binary32_test, double : binary64_test)(P)

int main(int argc, char *argv[]) {

  int p = atoi(argv[1]);

  REAL x;

  for (int i = 0; i < N; i++) {
    DO_TEST(x, p);
  }

  return 0;
}