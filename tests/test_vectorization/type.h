#ifndef __TYPE_H__
#define __TYPE_H__

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

#define XSTR(X) STR(X)
#define STR(X) #X

#define ZERO(X) _Generic(X, float : 0.0f, double : 0.0)
#define TWO(X) _Generic(X, float : 2.0f, double : 2.0)
#define THREE(X) _Generic(X, float : 3.0f, double : 3.0)

#define declare_arithmetic_vector_operation(type, operation, operator, size)   \
  type##size __attribute__((noinline))                                         \
      operation##_##size##x##_##type(type##size a, type x)

#define declare_arithmetic_vector_print(type, size)                            \
  void print_##size##x##_##type(type##size a, const char *name)

#define declare_comparison_vector_operation(type, operation, operator, size)   \
  int##size __attribute__((noinline))                                          \
      operation##_##size##x##_##type##_cmp(type##size a, int x)

#define declare_comparison_vector_print(size)                                  \
  void print_##size##x##_cmp(int##size a, const char *name)

declare_arithmetic_vector_print(float, 2);
declare_arithmetic_vector_print(float, 4);
declare_arithmetic_vector_print(float, 8);
declare_arithmetic_vector_print(float, 16);

declare_arithmetic_vector_print(double, 2);
declare_arithmetic_vector_print(double, 4);
declare_arithmetic_vector_print(double, 8);
declare_arithmetic_vector_print(double, 16);

declare_comparison_vector_print(2);
declare_comparison_vector_print(4);
declare_comparison_vector_print(8);
declare_comparison_vector_print(16);

/* Vectors size 2 */
declare_arithmetic_vector_operation(float, add, +, 2);
declare_arithmetic_vector_operation(float, sub, -, 2);
declare_arithmetic_vector_operation(float, mul, *, 2);
declare_arithmetic_vector_operation(float, div, /, 2);

declare_comparison_vector_operation(float, flt, <, 2);
declare_comparison_vector_operation(float, fle, <=, 2);
declare_comparison_vector_operation(float, fgt, >=, 2);
declare_comparison_vector_operation(float, fge, >, 2);
declare_comparison_vector_operation(float, feq, ==, 2);
declare_comparison_vector_operation(float, fneq, !=, 2);

declare_arithmetic_vector_operation(double, add, +, 2);
declare_arithmetic_vector_operation(double, sub, -, 2);
declare_arithmetic_vector_operation(double, mul, *, 2);
declare_arithmetic_vector_operation(double, div, /, 2);

declare_comparison_vector_operation(double, flt, <, 2);
declare_comparison_vector_operation(double, fle, <=, 2);
declare_comparison_vector_operation(double, fgt, >=, 2);
declare_comparison_vector_operation(double, fge, >, 2);
declare_comparison_vector_operation(double, feq, ==, 2);
declare_comparison_vector_operation(double, fneq, !=, 2);

/* Vectors size 4 */
declare_arithmetic_vector_operation(float, add, +, 4);
declare_arithmetic_vector_operation(float, sub, -, 4);
declare_arithmetic_vector_operation(float, mul, *, 4);
declare_arithmetic_vector_operation(float, div, /, 4);

declare_comparison_vector_operation(float, flt, <, 4);
declare_comparison_vector_operation(float, fle, <=, 4);
declare_comparison_vector_operation(float, fgt, >=, 4);
declare_comparison_vector_operation(float, fge, >, 4);
declare_comparison_vector_operation(float, feq, ==, 4);
declare_comparison_vector_operation(float, fneq, !=, 4);

declare_arithmetic_vector_operation(double, add, +, 4);
declare_arithmetic_vector_operation(double, sub, -, 4);
declare_arithmetic_vector_operation(double, mul, *, 4);
declare_arithmetic_vector_operation(double, div, /, 4);

declare_comparison_vector_operation(double, flt, <, 4);
declare_comparison_vector_operation(double, fle, <=, 4);
declare_comparison_vector_operation(double, fgt, >=, 4);
declare_comparison_vector_operation(double, fge, >, 4);
declare_comparison_vector_operation(double, feq, ==, 4);
declare_comparison_vector_operation(double, fneq, !=, 4);

/* Vectors size 8 */
declare_arithmetic_vector_operation(float, add, +, 8);
declare_arithmetic_vector_operation(float, sub, -, 8);
declare_arithmetic_vector_operation(float, mul, *, 8);
declare_arithmetic_vector_operation(float, div, /, 8);

declare_comparison_vector_operation(float, flt, <, 8);
declare_comparison_vector_operation(float, fle, <=, 8);
declare_comparison_vector_operation(float, fgt, >=, 8);
declare_comparison_vector_operation(float, fge, >, 8);
declare_comparison_vector_operation(float, feq, ==, 8);
declare_comparison_vector_operation(float, fneq, !=, 8);

declare_arithmetic_vector_operation(double, add, +, 8);
declare_arithmetic_vector_operation(double, sub, -, 8);
declare_arithmetic_vector_operation(double, mul, *, 8);
declare_arithmetic_vector_operation(double, div, /, 8);

declare_comparison_vector_operation(double, flt, <, 8);
declare_comparison_vector_operation(double, fle, <=, 8);
declare_comparison_vector_operation(double, fgt, >=, 8);
declare_comparison_vector_operation(double, fge, >, 8);
declare_comparison_vector_operation(double, feq, ==, 8);
declare_comparison_vector_operation(double, fneq, !=, 8);

/* Vectors size 16 */
declare_arithmetic_vector_operation(float, add, +, 16);
declare_arithmetic_vector_operation(float, sub, -, 16);
declare_arithmetic_vector_operation(float, mul, *, 16);
declare_arithmetic_vector_operation(float, div, /, 16);

declare_comparison_vector_operation(float, flt, <, 16);
declare_comparison_vector_operation(float, fle, <=, 16);
declare_comparison_vector_operation(float, fgt, >=, 16);
declare_comparison_vector_operation(float, fge, >, 16);
declare_comparison_vector_operation(float, feq, ==, 16);
declare_comparison_vector_operation(float, fneq, !=, 16);

declare_arithmetic_vector_operation(double, add, +, 16);
declare_arithmetic_vector_operation(double, sub, -, 16);
declare_arithmetic_vector_operation(double, mul, *, 16);
declare_arithmetic_vector_operation(double, div, /, 16);

declare_comparison_vector_operation(double, flt, <, 16);
declare_comparison_vector_operation(double, fle, <=, 16);
declare_comparison_vector_operation(double, fgt, >=, 16);
declare_comparison_vector_operation(double, fge, >, 16);
declare_comparison_vector_operation(double, feq, ==, 16);
declare_comparison_vector_operation(double, fneq, !=, 16);

#endif
