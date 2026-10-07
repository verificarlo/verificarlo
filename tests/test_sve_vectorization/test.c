/*
 * test_sve_vectorization/test.c
 *
 * Loops that the AArch64 loop vectorizer may turn into scalable vectors
 * (<vscale x N x T>) when SVE is enabled, e.g. with -march=native on an SVE
 * CPU. Verificarlo must instrument them.
 */
#include <stdio.h>

#define N 4096

float a[N], b[N], c[N];
double x[N], y[N], z[N];

__attribute__((noinline)) void addf(int n) {
  for (int i = 0; i < n; i++)
    c[i] = a[i] + b[i];
}

__attribute__((noinline)) void divd(int n) {
  for (int i = 0; i < n; i++)
    z[i] = x[i] / y[i];
}

int main(void) {
  for (int i = 0; i < N; i++) {
    a[i] = 0.1f * (float)i;
    b[i] = 1.0f / (float)(i + 3);
    x[i] = 0.1 * (double)i;
    y[i] = 3.0 + (double)i;
  }
  addf(N);
  divd(N);
  printf("%a %a\n", (double)c[N - 1], z[N - 1]);
  return 0;
}
