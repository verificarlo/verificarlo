#ifndef _INTERFLOP_IEEE_VECTOR_H_
#define _INTERFLOP_IEEE_VECTOR_H_

/* Define vector operation functions
 * size: size of vector
 * precision: floating point format (float or double)
 * ops: operation string (add, sub, mul, div)
 * ope: operator (+, -, *, /)
 */
#define define_interflop_op_vector(size, precision, ops, ope)                  \
  static void _interflop_##ops##_##precision##_##size##x(                      \
      const precision##size *a, const precision##size *b, precision##size *c,  \
      void *context) {                                                         \
    t_context *my_context = (t_context *)context;                              \
                                                                               \
    (*c) = (*a) ope (*b);                                                      \
                                                                               \
    my_context->_##size##x_##ops##_count++;                                    \
                                                                               \
    /* same code as 2 first line of debug_print_precision */                   \
    /* do this because we just need to test 1 times if the context say us      \
       to debug */                                                             \
    bool debug = ((t_context *)context)->debug ? true : false;                 \
    bool debug_binary = ((t_context *)context)->debug_binary ? true : false;   \
                                                                               \
    if (debug || debug_binary) {                                               \
      for (int i = 0; i < size; ++i) {                                         \
        debug_print_##precision(context, ARITHMETIC, #ope, (*a)[i], (*b)[i],   \
                                (*c)[i]);                                      \
      }                                                                        \
    }                                                                          \
  }

/* Define vector comparison functions
 * size: size of vector
 * precision: floating point format (float or double)
 */
#define define_interflop_cmp_vector(size, precision)                           \
  static void _interflop_cmp_##precision##_##size##x(                          \
      const enum FCMP_PREDICATE p, const precision##size *a,                   \
      const precision##size *b, int##size *c, void *context) {                 \
    for (int i = 0; i < size; ++i) {                                           \
      char *str = "";                                                          \
      int _c = (*c)[i];                                                        \
      SELECT_FLOAT_CMP((*a)[i], (*b)[i], &_c, p, str);                         \
      debug_print_##precision(context, COMPARISON, str, (*a)[i], (*b)[i],      \
                              (*c)[i]);                                        \
    }                                                                          \
  }

/* Define here all float vector interflop functions */
define_interflop_op_vector(2, float, add, +);
define_interflop_op_vector(2, float, sub, -);
define_interflop_op_vector(2, float, mul, *);
define_interflop_op_vector(2, float, div, /);

define_interflop_op_vector(4, float, add, +);
define_interflop_op_vector(4, float, sub, -);
define_interflop_op_vector(4, float, mul, *);
define_interflop_op_vector(4, float, div, /);

define_interflop_op_vector(8, float, add, +);
define_interflop_op_vector(8, float, sub, -);
define_interflop_op_vector(8, float, mul, *);
define_interflop_op_vector(8, float, div, /);

define_interflop_op_vector(16, float, add, +);
define_interflop_op_vector(16, float, sub, -);
define_interflop_op_vector(16, float, mul, *);
define_interflop_op_vector(16, float, div, /);

define_interflop_cmp_vector(2, float);
define_interflop_cmp_vector(4, float);
define_interflop_cmp_vector(8, float);
define_interflop_cmp_vector(16, float);

/* Define here all double vector interflop functions */
define_interflop_op_vector(2, double, add, +);
define_interflop_op_vector(2, double, sub, -);
define_interflop_op_vector(2, double, mul, *);
define_interflop_op_vector(2, double, div, /);

define_interflop_op_vector(4, double, add, +);
define_interflop_op_vector(4, double, sub, -);
define_interflop_op_vector(4, double, mul, *);
define_interflop_op_vector(4, double, div, /);

define_interflop_op_vector(8, double, add, +);
define_interflop_op_vector(8, double, sub, -);
define_interflop_op_vector(8, double, mul, *);
define_interflop_op_vector(8, double, div, /);

define_interflop_op_vector(16, double, add, +);
define_interflop_op_vector(16, double, sub, -);
define_interflop_op_vector(16, double, mul, *);
define_interflop_op_vector(16, double, div, /);

define_interflop_cmp_vector(2, double);
define_interflop_cmp_vector(4, double);
define_interflop_cmp_vector(8, double);
define_interflop_cmp_vector(16, double);

#endif // _INTERFLOP_IEEE_VECTOR_H_
