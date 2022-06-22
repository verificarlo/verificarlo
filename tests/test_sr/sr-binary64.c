#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  assert(argc == 2);
  int p = atoi(argv[1]);
  double a = 1.25;
  double b = pow(2, -p);

  for (int i = 0; i < ITERATIONS; i++) {
    printf("%.13a\n", a + b);
  }
  return 0;
}
