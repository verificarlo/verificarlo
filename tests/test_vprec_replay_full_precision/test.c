/* Regression test: replaying a VPREC profile at full precision must be the
 * identity.
 *
 * The precision stored in a profile counts significand bits (53 for binary64,
 * 24 for binary32), the same unit as --precision-binary64. The rounding
 * primitives take the stored mantissa bits, one fewer. The function
 * instrumentation used to hand its significand count straight to them, so at
 * full precision the mask was built with a negative shift and every rounded
 * argument came back with an all-zero mantissa.
 *
 * Exercises scalars and pointer arguments, in both binary32 and binary64.
 */
#include <stdio.h>
#include <stdlib.h>

__attribute__((noinline)) double accumulate(const double f[], int n) {
  double s = 0.0;
  for (int i = 0; i < n; i++) {
    s += f[i];
  }
  return s;
}

__attribute__((noinline)) float accumulatef(const float f[], int n) {
  float s = 0.0F;
  for (int i = 0; i < n; i++) {
    s += f[i];
  }
  return s;
}

__attribute__((noinline)) double scale(double a, double b) { return a * b; }

int main(int argc, char **argv) {
  int n = (argc > 1) ? atoi(argv[1]) : 64;

  double d[n];
  float f[n];
  for (int i = 0; i < n; i++) {
    d[i] = 1.0 / (double)(i + 3);
    f[i] = 1.0F / (float)(i + 3);
  }

  printf("%.16e\n", accumulate(d, n));
  printf("%.7e\n", accumulatef(f, n));
  printf("%.16e\n", scale(accumulate(d, n), 3.25));
  return 0;
}
