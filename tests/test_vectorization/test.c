#include <stdio.h>
#include <stdlib.h>

#include "type.h"

#define define_arithmetic_test(type, operation, size)                          \
  void run_test_##operation##_##type##_##size() {                              \
    type##size x;                                                              \
    type value;                                                                \
    for (int i = 0; i < size; i++) {                                           \
      x[i] = TWO(value);                                                       \
    }                                                                          \
    value = THREE(value);                                                      \
    type##size res = operation##_##size##x##_##type(x, value);                 \
    print_##size##x##_##type(res, STR(type##size));                            \
  }

#define define_comparison_test(type, operation, size)                          \
  void run_test_##operation##_##type##_##size() {                              \
    type##size x;                                                              \
    type value;                                                                \
    for (int i = 0; i < size; i++) {                                           \
      x[i] = TWO(value);                                                       \
    }                                                                          \
    value = THREE(value);                                                      \
    int##size res = operation##_##size##x##_##type##_cmp(x, value);            \
    print_##size##x##_cmp(res, STR(type##size));                               \
  }

define_arithmetic_test(float, add, 2);
define_arithmetic_test(double, add, 2);
define_arithmetic_test(float, sub, 2);
define_arithmetic_test(double, sub, 2);
define_arithmetic_test(float, mul, 2);
define_arithmetic_test(double, mul, 2);
define_arithmetic_test(float, div, 2);
define_arithmetic_test(double, div, 2);
define_comparison_test(float, flt, 2);
define_comparison_test(double, flt, 2);
define_comparison_test(float, fle, 2);
define_comparison_test(double, fle, 2);
define_comparison_test(float, fgt, 2);
define_comparison_test(double, fgt, 2);
define_comparison_test(float, fge, 2);
define_comparison_test(double, fge, 2);
define_comparison_test(float, feq, 2);
define_comparison_test(double, feq, 2);
define_comparison_test(float, fneq, 2);
define_comparison_test(double, fneq, 2);

#if defined(__x86_64__)
define_arithmetic_test(float, add, 4);
define_arithmetic_test(float, add, 8);
define_arithmetic_test(float, add, 16);

define_arithmetic_test(double, add, 4);
define_arithmetic_test(double, add, 8);
define_arithmetic_test(double, add, 16);

define_arithmetic_test(float, sub, 4);
define_arithmetic_test(float, sub, 8);
define_arithmetic_test(float, sub, 16);

define_arithmetic_test(double, sub, 4);
define_arithmetic_test(double, sub, 8);
define_arithmetic_test(double, sub, 16);

define_arithmetic_test(float, mul, 4);
define_arithmetic_test(float, mul, 8);
define_arithmetic_test(float, mul, 16);

define_arithmetic_test(double, mul, 4);
define_arithmetic_test(double, mul, 8);
define_arithmetic_test(double, mul, 16);

define_arithmetic_test(float, div, 4);
define_arithmetic_test(float, div, 8);
define_arithmetic_test(float, div, 16);

define_arithmetic_test(double, div, 4);
define_arithmetic_test(double, div, 8);
define_arithmetic_test(double, div, 16);

define_comparison_test(float, flt, 4);
define_comparison_test(float, flt, 8);
define_comparison_test(float, flt, 16);
define_comparison_test(double, flt, 4);
define_comparison_test(double, flt, 8);
define_comparison_test(double, flt, 16);

define_comparison_test(float, fle, 4);
define_comparison_test(float, fle, 8);
define_comparison_test(float, fle, 16);
define_comparison_test(double, fle, 4);
define_comparison_test(double, fle, 8);
define_comparison_test(double, fle, 16);

define_comparison_test(float, fgt, 4);
define_comparison_test(float, fgt, 8);
define_comparison_test(float, fgt, 16);
define_comparison_test(double, fgt, 4);
define_comparison_test(double, fgt, 8);
define_comparison_test(double, fgt, 16);

define_comparison_test(float, fge, 4);
define_comparison_test(float, fge, 8);
define_comparison_test(float, fge, 16);
define_comparison_test(double, fge, 4);
define_comparison_test(double, fge, 8);
define_comparison_test(double, fge, 16);

define_comparison_test(float, feq, 4);
define_comparison_test(float, feq, 8);
define_comparison_test(float, feq, 16);
define_comparison_test(double, feq, 4);
define_comparison_test(double, feq, 8);
define_comparison_test(double, feq, 16);

define_comparison_test(float, fneq, 4);
define_comparison_test(float, fneq, 8);
define_comparison_test(float, fneq, 16);
define_comparison_test(double, fneq, 4);
define_comparison_test(double, fneq, 8);
define_comparison_test(double, fneq, 16);
#endif

int main(int argc, char *argv[]) {

  run_test_add_float_2();
  run_test_add_double_2();
  run_test_sub_float_2();
  run_test_sub_double_2();
  run_test_mul_float_2();
  run_test_mul_double_2();
  run_test_div_float_2();
  run_test_div_double_2();
  run_test_flt_float_2();
  run_test_fle_float_2();
  run_test_fle_double_2();
  run_test_fgt_float_2();
  run_test_fgt_double_2();
  run_test_fge_float_2();
  run_test_fge_double_2();
  run_test_feq_float_2();
  run_test_feq_double_2();
  run_test_fneq_float_2();
  run_test_fneq_double_2();

#if defined(__x86_64__)
  run_test_add_float_4();
  run_test_add_float_8();
  run_test_add_float_16();

  run_test_add_double_4();
  run_test_add_double_8();
  run_test_add_double_16();

  run_test_sub_float_4();
  run_test_sub_float_8();
  run_test_sub_float_16();

  run_test_sub_double_4();
  run_test_sub_double_8();
  run_test_sub_double_16();

  run_test_mul_float_4();
  run_test_mul_float_8();
  run_test_mul_float_16();

  run_test_mul_double_4();
  run_test_mul_double_8();
  run_test_mul_double_16();

  run_test_div_float_4();
  run_test_div_float_8();
  run_test_div_float_16();

  run_test_div_double_4();
  run_test_div_double_8();
  run_test_div_double_16();

  run_test_flt_float_4();
  run_test_flt_float_8();
  run_test_flt_float_16();
  run_test_flt_double_2();
  run_test_flt_double_4();
  run_test_flt_double_8();
  run_test_flt_double_16();

  run_test_fle_float_4();
  run_test_fle_float_8();
  run_test_fle_float_16();

  run_test_fle_double_4();
  run_test_fle_double_8();
  run_test_fle_double_16();

  run_test_fgt_float_4();
  run_test_fgt_float_8();
  run_test_fgt_float_16();
  run_test_fgt_double_4();
  run_test_fgt_double_8();
  run_test_fgt_double_16();

  run_test_fge_float_4();
  run_test_fge_float_8();
  run_test_fge_float_16();
  run_test_fge_double_4();
  run_test_fge_double_8();
  run_test_fge_double_16();

  run_test_feq_float_4();
  run_test_feq_float_8();
  run_test_feq_float_16();
  run_test_feq_double_4();
  run_test_feq_double_8();
  run_test_feq_double_16();

  run_test_fneq_float_4();
  run_test_fneq_float_8();
  run_test_fneq_float_16();
  run_test_fneq_double_4();
  run_test_fneq_double_8();
  run_test_fneq_double_16();
#endif
  return 0;
}
