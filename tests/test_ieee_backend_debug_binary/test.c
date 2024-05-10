#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define STRTOFLT(x) _Generic(x, float : strtof, double : strtod)

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

  REAL x = STRTOFLT((REAL)0)(argv[1], NULL);
  REAL y = STRTOFLT((REAL)0)(argv[2], NULL);
  char op = argv[3][0];
  operation(x, y, op);

  return 0;
}
