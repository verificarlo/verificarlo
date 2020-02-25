#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef DOUBLE
#  define strtoflt(x) strtod(x,NULL)
#  define REAL double
#elif defined(FLOAT)
#  define strtoflt(x) strtof(x,NULL)
#  define REAL float
#else
#error "REAL type is not provided"
#endif

REAL operation(REAL x, REAL y, char op) {
  switch (op) {
  case '+':
    return x+y;
  case '-':
    return x-y;
  case '*':
    return x*y;
  case '/':
    return x/y;
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

  REAL x = strtoflt(argv[1]);
  REAL y = strtoflt(argv[2]);
  char op = argv[3][0];
  operation(x, y, op);
  
  return 0;
}
