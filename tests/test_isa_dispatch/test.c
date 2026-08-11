/*
 * test_isa_dispatch/test.c
 *
 * Vector source used to verify that the Verificarlo LLVM instrumentation
 * pass emits ISA-specific wrapper calls when the compilation target carries
 * an explicit ISA flag (-mavx, -mavx2, -mavx512f, -march=armv8-a, …).
 *
 * ext_vector_type produces explicit <N x float/double> LLVM IR regardless
 * of the optimisation level, which is what we need to trigger the vector
 * instrumentation path in the pass.
 */

#if defined(__x86_64__) || defined(__aarch64__)
typedef float  float4  __attribute__((ext_vector_type(4)));
typedef double double2 __attribute__((ext_vector_type(2)));

float4  vadd4f(float4 a, float4 b) { return a + b; }
float4  vsub4f(float4 a, float4 b) { return a - b; }
float4  vmul4f(float4 a, float4 b) { return a * b; }
float4  vdiv4f(float4 a, float4 b) { return a / b; }

double2 vadd2d(double2 a, double2 b) { return a + b; }
double2 vsub2d(double2 a, double2 b) { return a - b; }
double2 vmul2d(double2 a, double2 b) { return a * b; }
double2 vdiv2d(double2 a, double2 b) { return a / b; }
#endif
