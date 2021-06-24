#ifndef _INTERFLOP_VPREC_VECTOR_H_
#define _INTERFLOP_VPREC_VECTOR_H_

/******************** VPREC HELPER FUNCTIONS *******************
 * The following functions are used to set virtual precision,
 * VPREC mode of operation and instrumentation mode.
 ***************************************************************/

/* Vectorization using OpenCL */
#ifdef __clang__

/* Avoid warning about vector argument of type X without 'set' enabled changes
   the ABI */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpsabi"

#define define_compute_absErr_vprec_binary32_vector(size)                      \
  static inline int##size compute_absErr_vprec_binary32_##size##x(             \
      bool isDenormal, t_context *currentContext, int##size expDiff,           \
      int binary32_precision) {                                                \
                                                                               \
    /* this function is used only when in vprec error mode abs and all,        \
     * so there is no need to handle vprec error mode rel */                   \
    if (isDenormal == true) {                                                  \
      /* denormal, or underflow case */                                        \
      if (currentContext->relErr == true) {                                    \
        /* vprec error mode all */                                             \
        if (abs(currentContext->absErr_exp) < binary32_precision)              \
          return currentContext->absErr_exp;                                   \
        else                                                                   \
          return (int##size)binary32_precision;                                \
      } else {                                                                 \
        /* vprec error mode abs */                                             \
        return (int##size)currentContext->absErr_exp;                          \
      }                                                                        \
    } else {                                                                   \
      /* normal case */                                                        \
      if (currentContext->relErr == true) {                                    \
        /* vprec error mode all */                                             \
        int##size res = 0;                                                     \
        int##size res_test = expDiff < binary32_precision;                     \
                                                                               \
        for (int i = 0; i < size; i++) {                                       \
          if (res_test[i])                                                     \
            res[i] = expDiff[i];                                               \
          else {                                                               \
            res[i] = binary32_precision;                                       \
          }                                                                    \
        }                                                                      \
        return res;                                                            \
      } else {                                                                 \
        /* vprec error mode abs */                                             \
        int##size res = 0;                                                     \
        int##size res_test = expDiff < FLOAT_PMAN_SIZE;                        \
                                                                               \
        for (int i = 0; i < size; i++) {                                       \
          if (res_test[i])                                                     \
            res[i] = expDiff[i];                                               \
          else {                                                               \
            res[i] = FLOAT_PMAN_SIZE;                                          \
          }                                                                    \
        }                                                                      \
        return res;                                                            \
      }                                                                        \
    }                                                                          \
  }

define_compute_absErr_vprec_binary32_vector(2);
define_compute_absErr_vprec_binary32_vector(4);
define_compute_absErr_vprec_binary32_vector(8);
define_compute_absErr_vprec_binary32_vector(16);

#define define_compute_absErr_vprec_binary64_vector(size)                      \
  static inline int64_##size##x compute_absErr_vprec_binary64_##size##x(       \
      bool isDenormal, t_context *currentContext, int64_##size##x expDiff,     \
      int64_t binary64_precision) {                                            \
                                                                               \
    /* this function is used only when in vprec error mode abs and all,        \
     * so there is no need to handle vprec error mode rel */                   \
    if (isDenormal == true) {                                                  \
      /* denormal, or underflow case */                                        \
      if (currentContext->relErr == true) {                                    \
        /* vprec error mode all */                                             \
        if (abs(currentContext->absErr_exp) < binary64_precision)              \
          return currentContext->absErr_exp;                                   \
        else                                                                   \
          return (int64_##size##x)binary64_precision;                          \
      } else {                                                                 \
        /* vprec error mode abs */                                             \
        return (int64_##size##x)currentContext->absErr_exp;                    \
      }                                                                        \
    } else {                                                                   \
      /* normal case */                                                        \
      if (currentContext->relErr == true) {                                    \
        /* vprec error mode all */                                             \
        int64_##size##x res = 0;                                               \
        int64_##size##x res_test = expDiff < binary64_precision;               \
                                                                               \
        for (int i = 0; i < size; i++) {                                       \
          if (res_test[i])                                                     \
            res[i] = expDiff[i];                                               \
          else {                                                               \
            res[i] = binary64_precision;                                       \
          }                                                                    \
        }                                                                      \
        return res;                                                            \
      } else {                                                                 \
        /* vprec error mode abs */                                             \
        int64_##size##x res = 0;                                               \
        int64_##size##x res_test = expDiff < FLOAT_PMAN_SIZE;                  \
                                                                               \
        for (int i = 0; i < size; i++) {                                       \
          if (res_test[i])                                                     \
            res[i] = expDiff[i];                                               \
          else {                                                               \
            res[i] = FLOAT_PMAN_SIZE;                                          \
          }                                                                    \
        }                                                                      \
        return res;                                                            \
      }                                                                        \
    }                                                                          \
  }

define_compute_absErr_vprec_binary64_vector(2);
define_compute_absErr_vprec_binary64_vector(4);
define_compute_absErr_vprec_binary64_vector(8);
define_compute_absErr_vprec_binary64_vector(16);

// Macro to define vector function for normal absolute error mode
#define define_handle_binary32_normal_absErr_vector(size)                      \
  static inline void handle_binary32_normal_absErr_##size##x(                  \
      float##size *a, int##size aexp, int binary32_precision,                  \
      t_context *currentContext) {                                             \
                                                                               \
    /* absolute error mode, or both absolute and relative error modes */       \
    int##size expDiff = aexp - currentContext->absErr_exp;                     \
    float##size retVal;                                                        \
    int##size set = 0;                                                         \
    int count = 0;                                                             \
                                                                               \
    for (int i = 0; i < size; i++) {                                           \
      if (expDiff[i] < -1) {                                                   \
        /* equivalent to underflow on the precision given by absolute error */ \
        (*a)[i] = 0;                                                           \
        set[i] = 1;                                                            \
        count++;                                                               \
      } else if (expDiff[i] == -1) {                                           \
        /* case when the number is just below the absolute error threshold,    \
           but will round to one ulp on the format given by the absolute       \
           error; this needs to be handled separately, as                      \
           round_binary32_normal cannot generate this number */                \
        (*a)[i] = copysignf(exp2f(currentContext->absErr_exp), (*a)[i]);       \
        set[i] = 1;                                                            \
        count++;                                                               \
      }                                                                        \
    }                                                                          \
                                                                               \
    if (count == 0) { /* we can vectorize */                                   \
      /* normal case for the absolute error mode */                            \
      int##size binary32_precision_adjusted =                                  \
          compute_absErr_vprec_binary32_##size##x(                             \
              false, currentContext, expDiff, binary32_precision);             \
      round_binary32_normal_##size##x(a, binary32_precision_adjusted);         \
    } else { /* we can't vectorize */                                          \
      for (int i = 0; i < size; i++) {                                         \
        if (!set[i]) {                                                         \
          if (expDiff[i] < -1) {                                               \
            /* equivalent to underflow on the precision given by absolute      \
               error */                                                        \
            (*a)[i] = 0;                                                       \
          } else if (expDiff[i] == -1) {                                       \
            /* case when the number is just below the absolute error           \
               threshold, but will round to one ulp on the format given by the \
               absolute error; this needs to be handled separately,            \
               as round_binary32_normal cannot generate this number */         \
            (*a)[i] = copysignf(exp2f(currentContext->absErr_exp), (*a)[i]);   \
          }                                                                    \
        }                                                                      \
      }                                                                        \
    }                                                                          \
  }

// Declare all vector function for handle binary32 absolute error
define_handle_binary32_normal_absErr_vector(2);
define_handle_binary32_normal_absErr_vector(4);
define_handle_binary32_normal_absErr_vector(8);
define_handle_binary32_normal_absErr_vector(16);

// Macro to define vector function for normal absolute error mode
#define define_handle_binary64_normal_absErr_vector(size)                      \
  static inline void handle_binary64_normal_absErr_##size##x(                  \
      double##size *a, int64_##size##x aexp, int64_t binary64_precision,       \
      t_context *currentContext) {                                             \
                                                                               \
    /* absolute error mode, or both absolute and relative error modes */       \
    int64_##size##x expDiff = aexp - currentContext->absErr_exp;               \
    double##size retVal;                                                       \
    int##size set = 0;                                                         \
    int count = 0;                                                             \
                                                                               \
    for (int i = 0; i < size; i++) {                                           \
      if (expDiff[i] < -1) {                                                   \
        /* equivalent to underflow on the precision given by absolute error */ \
        (*a)[i] = 0;                                                           \
        set[i] = 1;                                                            \
        count++;                                                               \
      } else if (expDiff[i] == -1) {                                           \
        /* case when the number is just below the absolute error threshold,    \
           but will round to one ulp on the format given by the absolute       \
           error; this needs to be handled separately, as                      \
           round_binary64_normal cannot generate this number */                \
        (*a)[i] = copysignf(exp2f(currentContext->absErr_exp), (*a)[i]);       \
        set[i] = 1;                                                            \
        count++;                                                               \
      }                                                                        \
    }                                                                          \
                                                                               \
    if (count == 0) { /* we can vectorize */                                   \
      /* normal case for the absolute error mode */                            \
      int64_##size##x binary64_precision_adjusted =                            \
          compute_absErr_vprec_binary64_##size##x(                             \
              false, currentContext, expDiff, binary64_precision);             \
      round_binary64_normal_##size##x(a, binary64_precision_adjusted);         \
    } else { /* we can't vectorize */                                          \
      for (int i = 0; i < size; i++) {                                         \
        if (!set[i]) {                                                         \
          if (expDiff[i] < -1) {                                               \
            /* equivalent to underflow on the precision given by absolute      \
               error */                                                        \
            (*a)[i] = 0;                                                       \
          } else if (expDiff[i] == -1) {                                       \
            /* case when the number is just below the absolute error           \
               threshold, but will round to one ulp on the format given by the \
               absolute error; this needs to be handled separately,            \
               as round_binary64_normal cannot generate this number */         \
            (*a)[i] = copysignf(exp2f(currentContext->absErr_exp), (*a)[i]);   \
          }                                                                    \
        }                                                                      \
      }                                                                        \
    }                                                                          \
  }

// Declare all vector function for handle binary64 absolute error
define_handle_binary64_normal_absErr_vector(2);
define_handle_binary64_normal_absErr_vector(4);
define_handle_binary64_normal_absErr_vector(8);
define_handle_binary64_normal_absErr_vector(16);

/******************** VPREC ARITHMETIC FUNCTIONS ********************
 * The following set of functions perform the VPREC operation. Operands
 * are first correctly rounded to the target precison format if inbound
 * is set, the operation is then perform using IEEE hw and
 * correct rounding to the target precision format is done if outbound
 * is set.
 *******************************************************************/

#define perform_vector_binary_op(op, res, a, b)                                \
  perform_binary_op(op, *res, *a, *b)

// Round the float vector with the given precision
#define define_vprec_round_binary32_vector(size)                               \
  static inline void _vprec_round_binary32_##size##x(                          \
      float##size *a, char is_input, void *context, int binary32_range,        \
      int binary32_precision) {                                                \
                                                                               \
    t_context *currentContext = (t_context *)context;                          \
    int##size set = 0;                                                         \
    int count = 0;                                                             \
                                                                               \
    /* round to zero or set to infinity if underflow or overflow compare to */ \
    /* VPRECLIB_BINARY32_RANGE */                                              \
    int emax = (1 << (binary32_range - 1)) - 1;                                \
    /* here emin is the smallest exponent in the *normal* range */             \
    int emin = 1 - emax;                                                       \
                                                                               \
    binary32_##size##x aexp = {.f32 = *a};                                     \
    aexp.s32 = (int32_##size##x)((FLOAT_GET_EXP & aexp.u32));                  \
    aexp.s32 >>= (int32_##size##x)FLOAT_PMAN_SIZE;                             \
    aexp.s32 -= (int32_##size##x)FLOAT_EXP_COMP;                               \
                                                                               \
    /* make a vector test */                                                   \
    int##size is_overflow = aexp.s32 > (int##size)emax;                        \
                                                                               \
    for (int i = 0; i < size; i++) {                                           \
      /* test if 'a[i]' is a special case */                                   \
      if (!isfinite((*a)[i])) {                                                \
        set[i] = 1;                                                            \
        count++;                                                               \
      } /* check for overflow in target range */                               \
      else if (is_overflow[i]) {                                               \
        (*a)[i] = (*a)[i] * INFINITY;                                          \
        set[i] = 1;                                                            \
        count++;                                                               \
      } /* check for underflow in target range */                              \
      else if (aexp.s32[i] < emin) {                                           \
        /* underflow case: possibly a denormal */                              \
        if ((currentContext->daz && is_input) ||                               \
            (currentContext->ftz && !is_input)) {                              \
          /* preserve sign */                                                  \
          (*a)[i] = (*a)[i] * 0;                                               \
          set[i] = 1;                                                          \
          count++;                                                             \
        } else if (FP_ZERO == fpclassify((*a)[i])) {                           \
          set[i] = 1;                                                          \
          count++;                                                             \
        } else {                                                               \
          if (currentContext->absErr == true) {                                \
            /* absolute error mode, or both absolute and relative error        \
               modes */                                                        \
            int binary32_precision_adjusted = compute_absErr_vprec_binary32(   \
                true, currentContext, 0, binary32_precision);                  \
            (*a)[i] = handle_binary32_denormal((*a)[i], emin,                  \
                                               binary32_precision_adjusted);   \
          } else {                                                             \
            /* relative error mode */                                          \
            (*a)[i] =                                                          \
                handle_binary32_denormal((*a)[i], emin, binary32_precision);   \
          }                                                                    \
        }                                                                      \
        set[i] = 1;                                                            \
      }                                                                        \
    }                                                                          \
                                                                               \
    /* test if all vector is set */                                            \
    if (count == size) {                                                       \
      return;                                                                  \
    } else if (count != 0) {                                                   \
      /* if one element is set we can't vectorized */                          \
      for (int i = 0; i < size; i++) {                                         \
        if (!set[i]) {                                                         \
          /* else, normal case: can be executed even if a                      \
             previously rounded and truncated as denormal */                   \
          if (currentContext->absErr == true) {                                \
            /* absolute error mode, or both absolute and relative error        \
               modes */                                                        \
            (*a)[i] = handle_binary32_normal_absErr(                           \
                (*a)[i], aexp.s32[i], binary32_precision, currentContext);     \
          } else {                                                             \
            /* relative error mode */                                          \
            (*a)[i] = round_binary32_normal((*a)[i], binary32_precision);      \
          }                                                                    \
        }                                                                      \
      }                                                                        \
    } else {                                                                   \
      /* we can vectorize because we are sure that the vector is normal */     \
      /* else, normal case: can be executed even if a                          \
         previously rounded and truncated as denormal */                       \
      if (currentContext->absErr == true) {                                    \
        /* absolute error mode, or both absolute and relative error modes */   \
        handle_binary32_normal_absErr_##size##x(                               \
            a, aexp.s32, binary32_precision, currentContext);                  \
      } else {                                                                 \
        /* relative error mode */                                              \
        round_binary32_normal_##size##x(a, binary32_precision);                \
      }                                                                        \
    }                                                                          \
  }

// Declare all vector function for rounding binary32
define_vprec_round_binary32_vector(2);
define_vprec_round_binary32_vector(4);
define_vprec_round_binary32_vector(8);
define_vprec_round_binary32_vector(16);

// Round the double vector with the given precision
#define define_vprec_round_binary64_vector(size)                               \
  static inline void _vprec_round_binary64_##size##x(                          \
      double##size *a, char is_input, void *context, int binary64_range,       \
      int binary64_precision) {                                                \
                                                                               \
    t_context *currentContext = (t_context *)context;                          \
    int##size set = 0;                                                         \
    int count = 0;                                                             \
                                                                               \
    /* round to zero or set to infinity if underflow or overflow compare to */ \
    /* VPRECLIB_BINARY64_RANGE */                                              \
    int emax = (1 << (binary64_range - 1)) - 1;                                \
    /* here emin is the smallest exponent in the *normal* range */             \
    int emin = 1 - emax;                                                       \
                                                                               \
    binary64_##size##x aexp = {.f64 = *a};                                     \
    aexp.s64 = (int64_##size##x)((DOUBLE_GET_EXP & aexp.u64));                 \
    aexp.s64 >>= (int64_##size##x)DOUBLE_PMAN_SIZE;                            \
    aexp.s64 -= (int64_##size##x)DOUBLE_EXP_COMP;                              \
                                                                               \
    /* make a vector test */                                                   \
    int64_##size##x is_overflow = aexp.s64 > (int64_##size##x)emax;            \
                                                                               \
    for (int i = 0; i < size; i++) {                                           \
      /* test if 'a[i]' is a special case */                                   \
      if (!isfinite((*a)[i])) {                                                \
        set[i] = 1;                                                            \
        count++;                                                               \
      } /* check for overflow in target range */                               \
      else if (is_overflow[i]) {                                               \
        (*a)[i] = (*a)[i] * INFINITY;                                          \
        set[i] = 1;                                                            \
        count++;                                                               \
      } /* check for underflow in target range */                              \
      else if (aexp.s64[i] < emin) {                                           \
        /* underflow case: possibly a denormal */                              \
        if ((currentContext->daz && is_input) ||                               \
            (currentContext->ftz && !is_input)) {                              \
          /* preserve sign */                                                  \
          (*a)[i] = (*a)[i] * 0;                                               \
          set[i] = 1;                                                          \
          count++;                                                             \
        } else if (FP_ZERO == fpclassify((*a)[i])) {                           \
          set[i] = 1;                                                          \
          count++;                                                             \
        } else {                                                               \
          if (currentContext->absErr == true) {                                \
            /* absolute error mode, or both absolute and relative error        \
               modes */                                                        \
            int binary64_precision_adjusted = compute_absErr_vprec_binary64(   \
                true, currentContext, 0, binary64_precision);                  \
            (*a)[i] = handle_binary64_denormal((*a)[i], emin,                  \
                                               binary64_precision_adjusted);   \
          } else {                                                             \
            /* relative error mode */                                          \
            (*a)[i] =                                                          \
                handle_binary64_denormal((*a)[i], emin, binary64_precision);   \
          }                                                                    \
        }                                                                      \
        set[i] = 1;                                                            \
      }                                                                        \
    }                                                                          \
                                                                               \
    /* test if all vector is set */                                            \
    if (count == size) {                                                       \
      return;                                                                  \
    } else if (count != 0) {                                                   \
      /* if one element is set we can't vectorized */                          \
      for (int i = 0; i < size; i++) {                                         \
        if (!set[i]) {                                                         \
          /* else, normal case: can be executed even if a                      \
             previously rounded and truncated as denormal */                   \
          if (currentContext->absErr == true) {                                \
            /* absolute error mode, or both absolute and relative error        \
               modes */                                                        \
            (*a)[i] = handle_binary64_normal_absErr(                           \
                (*a)[i], aexp.s64[i], binary64_precision, currentContext);     \
          } else {                                                             \
            /* relative error mode */                                          \
            (*a)[i] = round_binary64_normal((*a)[i], binary64_precision);      \
          }                                                                    \
        }                                                                      \
      }                                                                        \
    } else {                                                                   \
      /* we can vectorize because we are sure that the vector is normal */     \
      /* else, normal case: can be executed even if a                          \
         previously rounded and truncated as denormal */                       \
      if (currentContext->absErr == true) {                                    \
        /* absolute error mode, or both absolute and relative error modes */   \
        handle_binary64_normal_absErr_##size##x(                               \
            a, aexp.s64, binary64_precision, currentContext);                  \
      } else {                                                                 \
        /* relative error mode */                                              \
        round_binary64_normal_##size##x(a, binary64_precision);                \
      }                                                                        \
    }                                                                          \
  }

// Declare all vector function for rounding binary64
define_vprec_round_binary64_vector(2);
define_vprec_round_binary64_vector(4);
define_vprec_round_binary64_vector(8);
define_vprec_round_binary64_vector(16);

/************************* FPHOOKS FUNCTIONS *************************
 * These functions correspond to those inserted into the source code
 * during source to source compilation and are replacement to floating
 * point operators
 **********************************************************************/

/* Macro to define all float vector interfop functions */
#define define_interflop_binary32_op_vector(size, op)                          \
  static void _interflop_##op##_float_##size##x(                               \
      float##size *a, float##size *b, float##size *c, void *context) {         \
    if ((VPRECLIB_MODE == vprecmode_full) ||                                   \
        (VPRECLIB_MODE == vprecmode_ib)) {                                     \
      _vprec_round_binary32_##size##x(a, 1, context, VPRECLIB_BINARY32_RANGE,  \
                                      VPRECLIB_BINARY32_PRECISION);            \
      _vprec_round_binary32_##size##x(b, 1, context, VPRECLIB_BINARY32_RANGE,  \
                                      VPRECLIB_BINARY32_PRECISION);            \
    }                                                                          \
                                                                               \
    perform_vector_binary_op(vprec_##op, c, a, b);                             \
                                                                               \
    if ((VPRECLIB_MODE == vprecmode_full) ||                                   \
        (VPRECLIB_MODE == vprecmode_ob)) {                                     \
      _vprec_round_binary32_##size##x(c, 0, context, VPRECLIB_BINARY32_RANGE,  \
                                      VPRECLIB_BINARY32_PRECISION);            \
    }                                                                          \
  }

/* Define here all float vector interflop functions */
define_interflop_binary32_op_vector(2, add);
define_interflop_binary32_op_vector(2, sub);
define_interflop_binary32_op_vector(2, mul);
define_interflop_binary32_op_vector(2, div);

define_interflop_binary32_op_vector(4, add);
define_interflop_binary32_op_vector(4, sub);
define_interflop_binary32_op_vector(4, mul);
define_interflop_binary32_op_vector(4, div);

define_interflop_binary32_op_vector(8, add);
define_interflop_binary32_op_vector(8, sub);
define_interflop_binary32_op_vector(8, mul);
define_interflop_binary32_op_vector(8, div);

define_interflop_binary32_op_vector(16, add);
define_interflop_binary32_op_vector(16, sub);
define_interflop_binary32_op_vector(16, mul);
define_interflop_binary32_op_vector(16, div);

/* Macro to define all double vector interfop functions */
#define define_interflop_binary64_op_vector(size, op)                          \
  static void _interflop_##op##_double_##size##x(                              \
      double##size *a, double##size *b, double##size *c, void *context) {      \
    if ((VPRECLIB_MODE == vprecmode_full) ||                                   \
        (VPRECLIB_MODE == vprecmode_ib)) {                                     \
      _vprec_round_binary64_##size##x(a, 1, context, VPRECLIB_BINARY64_RANGE,  \
                                      VPRECLIB_BINARY64_PRECISION);            \
      _vprec_round_binary64_##size##x(b, 1, context, VPRECLIB_BINARY64_RANGE,  \
                                      VPRECLIB_BINARY64_PRECISION);            \
    }                                                                          \
                                                                               \
    perform_vector_binary_op(vprec_##op, c, a, b);                             \
                                                                               \
    if ((VPRECLIB_MODE == vprecmode_full) ||                                   \
        (VPRECLIB_MODE == vprecmode_ob)) {                                     \
      _vprec_round_binary64_##size##x(c, 0, context, VPRECLIB_BINARY64_RANGE,  \
                                      VPRECLIB_BINARY64_PRECISION);            \
    }                                                                          \
  }

/* Define here all double vector interflop functions */
define_interflop_binary64_op_vector(2, add);
define_interflop_binary64_op_vector(2, sub);
define_interflop_binary64_op_vector(2, mul);
define_interflop_binary64_op_vector(2, div);

define_interflop_binary64_op_vector(4, add);
define_interflop_binary64_op_vector(4, sub);
define_interflop_binary64_op_vector(4, mul);
define_interflop_binary64_op_vector(4, div);

define_interflop_binary64_op_vector(8, add);
define_interflop_binary64_op_vector(8, sub);
define_interflop_binary64_op_vector(8, mul);
define_interflop_binary64_op_vector(8, div);

define_interflop_binary64_op_vector(16, add);
define_interflop_binary64_op_vector(16, sub);
define_interflop_binary64_op_vector(16, mul);
define_interflop_binary64_op_vector(16, div);

#endif // __clang__

#endif // _INTERFLOP_VPREC_VECTOR_H_
