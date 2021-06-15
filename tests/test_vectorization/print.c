#include "type.h"
#include <stdio.h>
#include <stdlib.h>

#define define_arithmetic_vector_print(type, size)                             \
  void print_##size##x##_##type(type##size a, const char *name) {              \
    for (int i = 0; i < size; i++) {                                           \
      fprintf(stderr, "%s[%d]=%f\n", name, i, a[i]);                           \
    }                                                                          \
  }

#define define_comparison_vector_print(size)                                   \
  void print_##size##x##_cmp(int##size a, const char *name) {                  \
    for (int i = 0; i < size; i++) {                                           \
      fprintf(stderr, "%s[%d]=%d\n", name, i, a[i]);                           \
    }                                                                          \
  }

define_comparison_vector_print(2);
define_comparison_vector_print(4);
define_comparison_vector_print(8);
define_comparison_vector_print(16);

define_arithmetic_vector_print(float, 2);
define_arithmetic_vector_print(float, 4);
define_arithmetic_vector_print(float, 8);
define_arithmetic_vector_print(float, 16);

define_arithmetic_vector_print(double, 2);
define_arithmetic_vector_print(double, 4);
define_arithmetic_vector_print(double, 8);
define_arithmetic_vector_print(double, 16);
