/*
 * test_vector_reduction/test.c
 *
 * Floating-point sum and product loops. The loop vectorizer turns them into
 * llvm.vector.reduce.fadd/fmul: with -ffast-math on every target, and even
 * without it on AArch64 (strict, in-order reductions). The arithmetic done by
 * these intrinsics must be instrumented.
 */
#include <stdio.h>

#define N 1024

float xf[N];
double xd[N];

__attribute__((noinline)) float sumf(int n) {
  float s = 0.0f;
  for (int i = 0; i < n; i++)
    s += xf[i];
  return s;
}

__attribute__((noinline)) double sumd(int n) {
  double s = 0.0;
  for (int i = 0; i < n; i++)
    s += xd[i];
  return s;
}

__attribute__((noinline)) double prodd(int n) {
  double p = 1.0;
  for (int i = 0; i < n; i++)
    p *= 1.0 + xd[i] / 64.0;
  return p;
}

int main(void) {
  for (int i = 0; i < N; i++) {
    xf[i] = 0.1f / (float)(i % 7 + 1);
    xd[i] = 0.1 / (double)(i % 7 + 1);
  }
  printf("%a %a %a\n", (double)sumf(N), sumd(N), prodd(N));
  return 0;
}
