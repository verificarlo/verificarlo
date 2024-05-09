#include <stdio.h>
#include <stdlib.h>

#define N 100

float dbl2flt(double a) { return (float)a; }

int main(int argc, char *argv[]) {

  if (argc < 2) {
    printf("Usage: %s <double>\n", argv[0]);
    return 1;
  }

  double a = atof(argv[1]);

  for (int i = 0; i < N; i++) {
    float b = dbl2flt(a);
    printf("%.6a\n", b);
  }
  return 0;
}