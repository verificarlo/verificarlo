#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef REAL
#error "REAL must be defined"
#endif

#define FMA(a, b, c)                                                           \
  _Generic((a), float : fmaf, double : fma, long double : fmal)(a, b, c)

int main(int argc, char *argv[]) {

  if (argc != 4) {
    fprintf(stderr, "Usage: %s <a> <b> <c>\n", argv[0]);
    return 1;
  }

  REAL a, b, c;
  a = atof(argv[1]);
  b = atof(argv[2]);
  c = atof(argv[3]);

  REAL result = FMA(a, b, c);

  printf("%.17e\n", result);

  return 0;
}