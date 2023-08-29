#include "type.h"

#define define_arithmetic_vector_operation(type, operation, operator, size)    \
  type##size __attribute__((noinline))                                         \
  operation##_##size##x##_##type(type##size a, type x) {                       \
    type##size res;                                                            \
    _Pragma("unroll") for (int i = 0; i < size; i++) {                         \
      res[i] = a[i] operator x;                                                \
    }                                                                          \
    return res;                                                                \
  }

#define define_comparison_vector_operation(type, operation, operator, size)    \
  int##size __attribute__((noinline))                                          \
  operation##_##size##x##_##type##_cmp(type##size a, int x) {                  \
    int##size res;                                                             \
    _Pragma("unroll") for (int i = 0; i < size; i++) {                         \
      int o = i % x;                                                           \
      res[i] = a[i] operator o;                                                \
    }                                                                          \
    return res;                                                                \
  }

/* Vector size 2  */
define_arithmetic_vector_operation(float, add, +, 2);
define_arithmetic_vector_operation(float, sub, -, 2);
define_arithmetic_vector_operation(float, mul, *, 2);
define_arithmetic_vector_operation(float, div, /, 2);

define_comparison_vector_operation(float, flt, <, 2);
define_comparison_vector_operation(float, fle, <=, 2);
define_comparison_vector_operation(float, fgt, >, 2);
define_comparison_vector_operation(float, fge, >=, 2);
define_comparison_vector_operation(float, feq, ==, 2);
define_comparison_vector_operation(float, fneq, !=, 2);

define_arithmetic_vector_operation(double, add, +, 2);
define_arithmetic_vector_operation(double, sub, -, 2);
define_arithmetic_vector_operation(double, mul, *, 2);
define_arithmetic_vector_operation(double, div, /, 2);

define_comparison_vector_operation(double, flt, <, 2);
define_comparison_vector_operation(double, fle, <=, 2);
define_comparison_vector_operation(double, fgt, >, 2);
define_comparison_vector_operation(double, fge, >=, 2);
define_comparison_vector_operation(double, feq, ==, 2);
define_comparison_vector_operation(double, fneq, !=, 2);

#if defined(__x86_64__)
/* Vector size 4  */
define_arithmetic_vector_operation(float, add, +, 4);
define_arithmetic_vector_operation(float, sub, -, 4);
define_arithmetic_vector_operation(float, mul, *, 4);
define_arithmetic_vector_operation(float, div, /, 4);

define_comparison_vector_operation(float, flt, <, 4);
define_comparison_vector_operation(float, fle, <=, 4);
define_comparison_vector_operation(float, fgt, >, 4);
define_comparison_vector_operation(float, fge, >=, 4);
define_comparison_vector_operation(float, feq, ==, 4);
define_comparison_vector_operation(float, fneq, !=, 4);

define_arithmetic_vector_operation(double, add, +, 4);
define_arithmetic_vector_operation(double, sub, -, 4);
define_arithmetic_vector_operation(double, mul, *, 4);
define_arithmetic_vector_operation(double, div, /, 4);

define_comparison_vector_operation(double, flt, <, 4);
define_comparison_vector_operation(double, fle, <=, 4);
define_comparison_vector_operation(double, fgt, >, 4);
define_comparison_vector_operation(double, fge, >=, 4);
define_comparison_vector_operation(double, feq, ==, 4);
define_comparison_vector_operation(double, fneq, !=, 4);

/* Vector size 8 */
define_arithmetic_vector_operation(float, add, +, 8);
define_arithmetic_vector_operation(float, sub, -, 8);
define_arithmetic_vector_operation(float, mul, *, 8);
define_arithmetic_vector_operation(float, div, /, 8);

define_comparison_vector_operation(float, flt, <, 8);
define_comparison_vector_operation(float, fle, <=, 8);
define_comparison_vector_operation(float, fgt, >, 8);
define_comparison_vector_operation(float, fge, >=, 8);
define_comparison_vector_operation(float, feq, ==, 8);
define_comparison_vector_operation(float, fneq, !=, 8);

define_arithmetic_vector_operation(double, add, +, 8);
define_arithmetic_vector_operation(double, sub, -, 8);
define_arithmetic_vector_operation(double, mul, *, 8);
define_arithmetic_vector_operation(double, div, /, 8);

define_comparison_vector_operation(double, flt, <, 8);
define_comparison_vector_operation(double, fle, <=, 8);
define_comparison_vector_operation(double, fgt, >, 8);
define_comparison_vector_operation(double, fge, >=, 8);
define_comparison_vector_operation(double, feq, ==, 8);
define_comparison_vector_operation(double, fneq, !=, 8);

/* Vector size 16 */
define_arithmetic_vector_operation(float, add, +, 16);
define_arithmetic_vector_operation(float, sub, -, 16);
define_arithmetic_vector_operation(float, mul, *, 16);
define_arithmetic_vector_operation(float, div, /, 16);

define_comparison_vector_operation(float, flt, <, 16);
define_comparison_vector_operation(float, fle, <=, 16);
define_comparison_vector_operation(float, fgt, >, 16);
define_comparison_vector_operation(float, fge, >=, 16);
define_comparison_vector_operation(float, feq, ==, 16);
define_comparison_vector_operation(float, fneq, !=, 16);

define_arithmetic_vector_operation(double, add, +, 16);
define_arithmetic_vector_operation(double, sub, -, 16);
define_arithmetic_vector_operation(double, mul, *, 16);
define_arithmetic_vector_operation(double, div, /, 16);

define_comparison_vector_operation(double, flt, <, 16);
define_comparison_vector_operation(double, fle, <=, 16);
define_comparison_vector_operation(double, fgt, >, 16);
define_comparison_vector_operation(double, fge, >=, 16);
define_comparison_vector_operation(double, feq, ==, 16);
define_comparison_vector_operation(double, fneq, !=, 16);
#endif
