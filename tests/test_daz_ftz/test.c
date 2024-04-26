#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define STRTOFLT(x) _Generic(x, float : strtof, double : strtod)
#define FMT(x) _Generic(x, float : "%.6a\n", double : "%.13a\n")

REAL operation(REAL x, REAL y, char op) {
  switch (op) {
  case '+':
    return x + y;
  case '-':
    return x - y;
  case '*':
    return x * y;
  case '/':
    return x / y;
  default:
    fprintf(stderr, "Invalid operation %c\n", op);
    exit(1);
  }
}

int main(int argc, char *argv[]) {

  if (argc != 4) {
    fprintf(stderr, "usage: <float> <float> op\n");
    exit(1);
  }

  char op;
  REAL x, y, z;
  x = STRTOFLT(x)(argv[1], NULL);
  y = STRTOFLT(y)(argv[2], NULL);
  op = argv[3][0];
  z = operation(x, y, op);

  const char *fmt = FMT(x);
  printf(fmt, z);
  return 0;
}
