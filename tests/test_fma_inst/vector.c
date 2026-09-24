#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef REAL
#error "REAL must be defined"
#endif

#define FMA(a, b, c)                                                           \
  _Generic((a), float : fmaf, double : fma, long double : fmal)(a, b, c)

#define N 1024

/* At -O3 the loop vectorizer turns this into llvm.fma.v<N><type> */
__attribute__((noinline)) void kernel(const REAL *restrict a,
                                      const REAL *restrict b,
                                      const REAL *restrict c,
                                      REAL *restrict o) {
  for (int i = 0; i < N; i++)
    o[i] = FMA(a[i], b[i], c[i]);
}

int main(int argc, char *argv[]) {

  if (argc != 4) {
    fprintf(stderr, "Usage: %s <a> <b> <c>\n", argv[0]);
    return 1;
  }

  static REAL a[N], b[N], c[N], o[N];
  for (int i = 0; i < N; i++) {
    a[i] = atof(argv[1]);
    b[i] = atof(argv[2]);
    c[i] = atof(argv[3]);
  }

  kernel(a, b, c, o);

  /* Print a lane from the vector body, not the scalar remainder */
  printf("%.17e\n", o[N / 2]);

  return 0;
}
