/*
 * test_wide_vector_abi/test.c
 *
 * Vector operations wider than the target vector registers (256/512 bits on
 * 128-bit NEON/SVE/SSE2), whose operands the instrumentation pass must pass to
 * the MCA wrappers following the target calling convention. With the IEEE
 * backend, the instrumented program must print exactly the native results.
 */
#include <stdio.h>

typedef float float8 __attribute__((ext_vector_type(8)));
typedef double double4 __attribute__((ext_vector_type(4)));
typedef double double8 __attribute__((ext_vector_type(8)));
typedef int int8 __attribute__((ext_vector_type(8)));
typedef long long4 __attribute__((ext_vector_type(4)));
typedef long long8 __attribute__((ext_vector_type(8)));

__attribute__((noinline)) long4 lt4d(double4 a, double4 b) { return a < b; }
__attribute__((noinline)) long8 le8d(double8 a, double8 b) { return a <= b; }
__attribute__((noinline)) int8 gt8f(float8 a, float8 b) { return a > b; }
__attribute__((noinline)) double4 add4d(double4 a, double4 b) { return a + b; }
__attribute__((noinline)) float8 mul8f(float8 a, float8 b) { return a * b; }

int main(void) {
  double4 a4 = {0.1, 0.2, 0.3, 0.4}, b4 = {0.4, 0.1, 0.5, 0.2};
  double8 a8 = {1, 2, 3, 4, 5, 6, 7, 8}, b8 = {8, 2, 6, 4, 4, 6, 2, 9};
  float8 af = {1, 2, 3, 4, 5, 6, 7, 8}, bf = {0.5f, 3, 2, 5, 5, 1, 9, 7};

  long4 r1 = lt4d(a4, b4);
  long8 r2 = le8d(a8, b8);
  int8 r3 = gt8f(af, bf);
  double4 r4 = add4d(a4, b4);
  float8 r5 = mul8f(af, bf);

  for (int i = 0; i < 4; i++)
    printf("%ld ", r1[i]);
  for (int i = 0; i < 8; i++)
    printf("%ld ", r2[i]);
  for (int i = 0; i < 8; i++)
    printf("%d ", r3[i]);
  for (int i = 0; i < 4; i++)
    printf("%a ", r4[i]);
  for (int i = 0; i < 8; i++)
    printf("%a ", (double)r5[i]);
  printf("\n");
  return 0;
}
