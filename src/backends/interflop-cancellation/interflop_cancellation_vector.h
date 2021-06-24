#ifndef _INTERFLOP_CANCELLATION_VECTOR_H_
#define _INTERFLOP_CANCELLATION_VECTOR_H_

#define define_interflop_add_sub_vector(size, precision, ops, ope)             \
  static void _interflop_##ops##_##precision##_##size##x(                      \
      precision##size *a, precision##size *b, precision##size *c,              \
      void *context) {                                                         \
    (*c) = (*a)ope(*b);                                                        \
    for (int i = 0; i < size; ++i) {                                           \
      precision _c = (*c)[i];                                                  \
      cancell((*a)[i], (*b)[i], &_c);                                          \
      (*c)[i] = _c;                                                            \
    }                                                                          \
  }

#define define_interflop_mul_div_vector(size, precision, ops, ope)             \
  static void _interflop_##ops##_##precision##_##size##x(                      \
      precision##size *a, precision##size *b, precision##size *c,              \
      void *context) {                                                         \
    (*c) = (*a)ope(*b);                                                        \
  }

/* Define float add and sub vector functions */
define_interflop_add_sub_vector(2, float, add, +);
define_interflop_add_sub_vector(2, float, sub, -);

define_interflop_add_sub_vector(4, float, add, +);
define_interflop_add_sub_vector(4, float, sub, -);

define_interflop_add_sub_vector(8, float, add, +);
define_interflop_add_sub_vector(8, float, sub, -);

define_interflop_add_sub_vector(16, float, add, +);
define_interflop_add_sub_vector(16, float, sub, -);

/* Define double add and sub vector functions */
define_interflop_add_sub_vector(2, double, add, +);
define_interflop_add_sub_vector(2, double, sub, -);

define_interflop_add_sub_vector(4, double, add, +);
define_interflop_add_sub_vector(4, double, sub, -);

define_interflop_add_sub_vector(8, double, add, +);
define_interflop_add_sub_vector(8, double, sub, -);

define_interflop_add_sub_vector(16, double, add, +);
define_interflop_add_sub_vector(16, double, sub, -);

/* Define float mul and div vector functions */
define_interflop_mul_div_vector(2, float, mul, *);
define_interflop_mul_div_vector(2, float, div, /);

define_interflop_mul_div_vector(4, float, mul, *);
define_interflop_mul_div_vector(4, float, div, /);

define_interflop_mul_div_vector(8, float, mul, *);
define_interflop_mul_div_vector(8, float, div, /);

define_interflop_mul_div_vector(16, float, mul, *);
define_interflop_mul_div_vector(16, float, div, /);

/* Define double mul and div vector functions */
define_interflop_mul_div_vector(2, double, mul, *);
define_interflop_mul_div_vector(2, double, div, /);

define_interflop_mul_div_vector(4, double, mul, *);
define_interflop_mul_div_vector(4, double, div, /);

define_interflop_mul_div_vector(8, double, mul, *);
define_interflop_mul_div_vector(8, double, div, /);

define_interflop_mul_div_vector(16, double, mul, *);
define_interflop_mul_div_vector(16, double, div, /);

#endif // _INTERFLOP_CANCELLATION_VECTOR_H_
