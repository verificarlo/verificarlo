#include<stdbool.h>
bool f_ogt(double a, double b) {
  return (a>b);
}

bool f_ole(double a, double b) {
  return (a<=b);
}

// vectorized comparisons in aarch64 are not supported in LLVM15
#if (__x86_64__)
typedef double double4 __attribute__((ext_vector_type(4)));
typedef long long4 __attribute__((ext_vector_type(4)));

long4 f4x_ogt(double4 a, double4 b) {
  return (a > b);
}
#endif
