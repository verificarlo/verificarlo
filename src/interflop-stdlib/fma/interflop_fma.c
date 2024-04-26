#include <quadmath.h>

#include "interflop_fma.h"

float interflop_fma_binary32(float a, float b, float c) {
  return __builtin_fmaf(a, b, c);
}

double interflop_fma_binary64(double a, double b, double c) {
  return __builtin_fma(a, b, c);
}

_Float128 interflop_fma_binary128(_Float128 a, _Float128 b, _Float128 c) {
  return fmaq(a, b, c);
}

int main(int argc, char *argv) { return 0; }