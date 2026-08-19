#ifndef __VFC_OPERATOR_H__
#define __VFC_OPERATOR_H__

#include "interflop/interflop.h"

#ifdef __GNUC__
typedef float float2 __attribute__((vector_size(8)));
typedef float float4 __attribute__((vector_size(16)));
typedef float float8 __attribute__((vector_size(32)));
typedef float float16 __attribute__((vector_size(64)));
typedef double double2 __attribute__((vector_size(16)));
typedef double double4 __attribute__((vector_size(32)));
typedef double double8 __attribute__((vector_size(64)));
typedef double double16 __attribute__((vector_size(128)));
typedef int int2 __attribute__((vector_size(8)));
typedef int int4 __attribute__((vector_size(16)));
typedef int int8 __attribute__((vector_size(32)));
typedef int int16 __attribute__((vector_size(64)));
#elif __clang__
typedef double double2 __attribute__((ext_vector_type(2)));
typedef double double4 __attribute__((ext_vector_type(4)));
typedef double double8 __attribute__((ext_vector_type(8)));
typedef double double16 __attribute__((ext_vector_type(16)));
typedef float float2 __attribute__((ext_vector_type(2)));
typedef float float4 __attribute__((ext_vector_type(4)));
typedef float float8 __attribute__((ext_vector_type(8)));
typedef float float16 __attribute__((ext_vector_type(16)));
typedef int int2 __attribute__((ext_vector_type(2)));
typedef int int4 __attribute__((ext_vector_type(4)));
typedef int int8 __attribute__((ext_vector_type(8)));
typedef int int16 __attribute__((ext_vector_type(16)));
#else
#error "Compiler must be gcc or clang"
#endif

#define define_arithmetic_wrapper(precision, operation, operator)              \
  precision _##precision##operation(precision a, precision b);

define_arithmetic_wrapper(float, add, (a + b));
define_arithmetic_wrapper(float, sub, (a - b));
define_arithmetic_wrapper(float, mul, (a * b));
define_arithmetic_wrapper(float, div, (a / b));
define_arithmetic_wrapper(double, add, (a + b));
define_arithmetic_wrapper(double, sub, (a - b));
define_arithmetic_wrapper(double, mul, (a * b));
define_arithmetic_wrapper(double, div, (a / b));

int _floatcmp(enum FCMP_PREDICATE p, float a, float b);
int _doublecmp(enum FCMP_PREDICATE p, double a, double b);

/* Arithmetic vector wrappers */
#define define_vectorized_arithmetic_wrapper(precision, operation, size)       \
  precision##size _##size##x##precision##operation(const precision##size a,    \
                                                   const precision##size b);

/* Define vector of size 2 */
define_vectorized_arithmetic_wrapper(float, add, 2);
define_vectorized_arithmetic_wrapper(float, sub, 2);
define_vectorized_arithmetic_wrapper(float, mul, 2);
define_vectorized_arithmetic_wrapper(float, div, 2);
define_vectorized_arithmetic_wrapper(double, add, 2);
define_vectorized_arithmetic_wrapper(double, sub, 2);
define_vectorized_arithmetic_wrapper(double, mul, 2);
define_vectorized_arithmetic_wrapper(double, div, 2);

/* Define vector of size 4 */
define_vectorized_arithmetic_wrapper(float, add, 4);
define_vectorized_arithmetic_wrapper(float, sub, 4);
define_vectorized_arithmetic_wrapper(float, mul, 4);
define_vectorized_arithmetic_wrapper(float, div, 4);
define_vectorized_arithmetic_wrapper(double, add, 4);
define_vectorized_arithmetic_wrapper(double, sub, 4);
define_vectorized_arithmetic_wrapper(double, mul, 4);
define_vectorized_arithmetic_wrapper(double, div, 4);

/* Define vector of size 8 */
define_vectorized_arithmetic_wrapper(float, add, 8);
define_vectorized_arithmetic_wrapper(float, sub, 8);
define_vectorized_arithmetic_wrapper(float, mul, 8);
define_vectorized_arithmetic_wrapper(float, div, 8);
define_vectorized_arithmetic_wrapper(double, add, 8);
define_vectorized_arithmetic_wrapper(double, sub, 8);
define_vectorized_arithmetic_wrapper(double, mul, 8);
define_vectorized_arithmetic_wrapper(double, div, 8);

/* Define vector of size 16 */
define_vectorized_arithmetic_wrapper(float, add, 16);
define_vectorized_arithmetic_wrapper(float, sub, 16);
define_vectorized_arithmetic_wrapper(float, mul, 16);
define_vectorized_arithmetic_wrapper(float, div, 16);
define_vectorized_arithmetic_wrapper(double, add, 16);
define_vectorized_arithmetic_wrapper(double, sub, 16);
define_vectorized_arithmetic_wrapper(double, mul, 16);
define_vectorized_arithmetic_wrapper(double, div, 16);

/* Comparison vector wrappers */
#define define_vectorized_comparison_wrapper(precision, size)                  \
  int##size _##size##x##precision##cmp(enum FCMP_PREDICATE p,                  \
                                       precision##size a, precision##size b);

define_vectorized_comparison_wrapper(float, 2);
define_vectorized_comparison_wrapper(double, 2);

define_vectorized_comparison_wrapper(float, 4);
define_vectorized_comparison_wrapper(double, 4);

define_vectorized_comparison_wrapper(float, 8);
define_vectorized_comparison_wrapper(double, 8);

define_vectorized_comparison_wrapper(float, 16);
define_vectorized_comparison_wrapper(double, 16);

#define define_arithmetic_fma_wrapper(precision)                               \
  precision _##precision##fma(precision a, precision b, precision c);

define_arithmetic_fma_wrapper(float);
define_arithmetic_fma_wrapper(double);

float _doubletofloatcast(double a);

#endif /* __VFC_OPERATOR_H__ */
