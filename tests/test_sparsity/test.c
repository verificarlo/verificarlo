#include <stdio.h>
#include <stdlib.h>

#define FMT(X) _Generic((X), double : strtod, float : strtof)

int main(int argc, char *argv[]) {
  REAL sum = 0.0;
  REAL a = FMT(sum)(argv[1], NULL);
  REAL b = FMT(sum)(argv[2], NULL);
  for (int i = 0; i < 1000; i++) {
    sum = a + b;
    printf("%.13a\n", sum);
  }
}
