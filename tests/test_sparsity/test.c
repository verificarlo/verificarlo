#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  double a = strtod(argv[1], NULL);
  double b = strtod(argv[2], NULL);
  double sum = 0.0;
  for (int i = 0; i < 100; i++) {
    sum = a + b;
    printf("%.13a\n", sum);
  }
}
