#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "../../interflop_stdlib.c"

int main() {

  for (int i = -150; i < 128; i++) {
    float a = fpow2i(i);
    float b = exp2f(i);
    if (b - a != 0) {
      fprintf(stderr, "==================\n");
      fprintf(stderr, "i     : %d\n", i);
      fprintf(stderr, "pow2if: %.6a\n", a);
      fprintf(stderr, "exp2f : %.6a\n", b);
      fprintf(stderr, "eabs  : %.6a\n", b - a);
    }
    assert(b - a == 0);
  }

  for (int i = -1050; i < 1024; i++) {
    double a = pow2i(i);
    double b = exp2(i);
    if (b - a != 0) {
      fprintf(stderr, "==================\n");
      fprintf(stderr, "i    : %d\n", i);
      fprintf(stderr, "pow2i: %.13a\n", a);
      fprintf(stderr, "exp2 : %.13a\n", b);
      fprintf(stderr, "eabs : %.13a\n", b - a);
    }
    assert(b - a == 0);
  }

  fprintf(stderr, "Test passed\n");
}