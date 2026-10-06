#include <stdio.h>
#include <stdlib.h>

/* a * b + c in C is contracted by clang into llvm.fmuladd (-ffp-contract=on
 * is the default), scalar at -O0 and vectorized at -O3. */

#define N 1024

#ifndef REAL
#define REAL double
#endif

REAL a[N], b[N], c[N], r[N];

__attribute__((noinline)) void muladd(int n) {
  for (int i = 0; i < n; i++) {
    r[i] = a[i] * b[i] + c[i];
  }
}

int main(int argc, char *argv[]) {
  for (int i = 0; i < N; i++) {
    a[i] = (REAL)0.1;
    b[i] = (REAL)1 / (REAL)(i + 3);
    c[i] = (REAL)0.01;
  }
  muladd(N);
  printf("%a\n", (double)r[N - 1]);
  return 0;
}
