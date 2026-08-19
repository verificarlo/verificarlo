/*
 * interflop_vector_backend.h
 *
 * Target-versioned vectorized wrappers for Interflop/Verificarlo.
 * Generates one version per ABI-relevant ISA level:
 *   - AVX-512  (+avx512f)
 *   - AVX2     (+avx2)
 *   - AVX      (+avx)
 *   - SSE2     (+sse2)  [baseline x86-64]
 *
 * ABI rule: vector passing ABI changes only at SSE2 / AVX / AVX512F
 * boundaries — not at AVX2, SSE4.x, AVX512BW/VL, etc.
 *
 * The LLVM instrumentation pass selects the right version by matching
 * the caller's target-features against the clone's attributes.
 */

#pragma once

#include <stdint.h>

#include "backends.h"
#include "interflop/interflop.h"
#include "vector_ext.h"

/* =========================================================================
 * ISA target descriptors
 * ========================================================================= */

/* Attribute strings used by the LLVM pass when cloning:
 *   INTERFLOP_TARGET_FEATURES_AVX512
 *   INTERFLOP_TARGET_FEATURES_AVX2
 *   INTERFLOP_TARGET_FEATURES_AVX
 *   INTERFLOP_TARGET_FEATURES_SSE2
 */
#define INTERFLOP_TARGET_FEATURES_AVX512 "+avx512f"
#define INTERFLOP_TARGET_FEATURES_AVX2 "+avx,+avx2"
#define INTERFLOP_TARGET_FEATURES_AVX "+avx"
#define INTERFLOP_TARGET_FEATURES_SSE2 "+sse2"

/* =========================================================================
 * Loop unroll hint (from your existing code)
 * ========================================================================= */

#define DO_PRAGMA(x) _Pragma(#x)

#if defined(__clang__)
#define UNROLL(n) DO_PRAGMA(clang loop unroll_count(n))
#elif defined(__GNUC__)
#define UNROLL(n) DO_PRAGMA(GCC unroll n)
#else
#define UNROLL(n)
#endif

/* =========================================================================
 * Per-target attribute macros
 * Clang/GCC: __attribute__((target("...")))
 * ========================================================================= */

#define TARGET_AVX512 __attribute__((target("avx512f")))
#define TARGET_AVX2 __attribute__((target("avx2,fma")))
#define TARGET_AVX __attribute__((target("avx")))
#define TARGET_SSE2 __attribute__((target("sse2")))

/* =========================================================================
 * Core macro: emit one arithmetic vector wrapper for a given target level
 *
 *  TARGET_ATTR  : one of TARGET_AVX512 / TARGET_AVX2 / TARGET_AVX / TARGET_SSE2
 *  TARGET_SUFFIX: avx512 / avx2 / avx / sse2
 *  precision    : float / double
 *  operation    : add / sub / mul / div
 *  size         : 2 / 4 / 8 / 16
 * ========================================================================= */

#define define_vector_wrapper_for_target(TARGET_ATTR, TARGET_SUFFIX,           \
                                         precision, operation, size)           \
  TARGET_ATTR                                                                  \
  precision##size _##size##x##precision##operation##_##TARGET_SUFFIX(          \
      const precision##size a, const precision##size b) {                      \
    typedef union {                                                            \
      precision##size v;                                                       \
      precision a[size];                                                       \
    } fvec;                                                                    \
    fvec au = {.v = a}, bu = {.v = b}, cu;                                     \
    for (int i = 0; i < loaded_backends; i++) {                                \
      if (backends[i].interflop_##operation##_##precision) {                   \
        UNROLL(size)                                                           \
        for (int j = 0; j < (size); j++) {                                     \
          backends[i].interflop_##operation##_##precision(                     \
              au.a[j], bu.a[j], &(cu.a[j]), contexts[i]);                      \
        }                                                                      \
      }                                                                        \
    }                                                                          \
    return cu.v;                                                               \
  }

/* Emit all 4 target variants for one (precision, operation, size) triple */
#define define_vector_wrapper_all_targets(precision, operation, size)          \
  define_vector_wrapper_for_target(TARGET_AVX512, avx512, precision,           \
                                   operation, size)                            \
      define_vector_wrapper_for_target(TARGET_AVX2, avx2, precision,           \
                                       operation, size)                        \
          define_vector_wrapper_for_target(TARGET_AVX, avx, precision,         \
                                           operation, size)                    \
              define_vector_wrapper_for_target(TARGET_SSE2, sse2, precision,   \
                                               operation, size)

/* =========================================================================
 * Core macro: emit one comparison vector wrapper for a given target level
 * ========================================================================= */

#define define_vector_cmp_for_target(TARGET_ATTR, TARGET_SUFFIX, precision,    \
                                     size)                                     \
  TARGET_ATTR                                                                  \
  int##size _##size##x##precision##cmp##_##TARGET_SUFFIX(                      \
      enum FCMP_PREDICATE p, precision##size a, precision##size b) {           \
    typedef union {                                                            \
      int##size v;                                                             \
      int a[size];                                                             \
    } ivec;                                                                    \
    typedef union {                                                            \
      precision##size v;                                                       \
      precision a[size];                                                       \
    } fvec;                                                                    \
    fvec au = {.v = a}, bu = {.v = b};                                         \
    ivec cu;                                                                   \
    unsigned char implemented = 0;                                             \
    for (int i = 0; i < loaded_backends; i++) {                                \
      if (backends[i].interflop_cmp_##precision) {                             \
        implemented = 1;                                                       \
        UNROLL(size)                                                           \
        for (int j = 0; j < (size); j++) {                                     \
          backends[i].interflop_cmp_##precision(p, au.a[j], bu.a[j],           \
                                                &(cu.a[j]), contexts[i]);      \
        }                                                                      \
      }                                                                        \
    }                                                                          \
    require_backend_implements(implemented, precision, cmp);                   \
    return cu.v;                                                               \
  }

#define define_vector_cmp_all_targets(precision, size)                         \
  define_vector_cmp_for_target(TARGET_AVX512, avx512, precision, size)         \
      define_vector_cmp_for_target(TARGET_AVX2, avx2, precision, size)         \
          define_vector_cmp_for_target(TARGET_AVX, avx, precision, size)       \
              define_vector_cmp_for_target(TARGET_SSE2, sse2, precision, size)

/* =========================================================================
 * FMA vector wrapper for a given target level
 * ========================================================================= */

#define define_vector_fma_for_target(TARGET_ATTR, TARGET_SUFFIX, precision,    \
                                     size)                                     \
  TARGET_ATTR                                                                  \
  precision##size _##size##x##precision##fma##_##TARGET_SUFFIX(                \
      const precision##size a, const precision##size b,                        \
      const precision##size c) {                                               \
    typedef union {                                                            \
      precision##size v;                                                       \
      precision a[size];                                                       \
    } fvec;                                                                    \
    fvec au = {.v = a};                                                        \
    fvec bu = {.v = b};                                                        \
    fvec cu = {.v = c};                                                        \
    fvec du;                                                                   \
    unsigned char implemented = 0;                                             \
    for (int i = 0; i < loaded_backends; i++) {                                \
      if (backends[i].interflop_fma_##precision) {                             \
        implemented = 1;                                                       \
        UNROLL(size)                                                           \
        for (int j = 0; j < (size); j++) {                                     \
          backends[i].interflop_fma_##precision(au.a[j], bu.a[j], cu.a[j],     \
                                                &(du.a[j]), contexts[i]);      \
        }                                                                      \
      }                                                                        \
    }                                                                          \
    require_backend_implements(implemented, precision, fma);                   \
    return du.v;                                                               \
  }

#define define_vector_fma_all_targets(precision, size)                         \
  define_vector_fma_for_target(TARGET_AVX512, avx512, precision, size)         \
      define_vector_fma_for_target(TARGET_AVX2, avx2, precision, size)         \
          define_vector_fma_for_target(TARGET_AVX, avx, precision, size)       \
              define_vector_fma_for_target(TARGET_SSE2, sse2, precision, size)

/* =========================================================================
 * Instantiate all wrappers for all (precision, operation, size) combinations
 * ========================================================================= */

/* --- float arithmetic --- */
define_vector_wrapper_all_targets(float, add,
                                  2) define_vector_wrapper_all_targets(float,
                                                                       sub, 2)
    define_vector_wrapper_all_targets(
        float, mul, 2) define_vector_wrapper_all_targets(float, div, 2)

        define_vector_wrapper_all_targets(
            float, add, 4) define_vector_wrapper_all_targets(float, sub, 4)
            define_vector_wrapper_all_targets(
                float, mul, 4) define_vector_wrapper_all_targets(float, div, 4)

                define_vector_wrapper_all_targets(float, add, 8)
                    define_vector_wrapper_all_targets(float, sub, 8)
                        define_vector_wrapper_all_targets(float, mul, 8)
                            define_vector_wrapper_all_targets(float, div, 8)

                                define_vector_wrapper_all_targets(float, add,
                                                                  16)
                                    define_vector_wrapper_all_targets(float,
                                                                      sub, 16)
                                        define_vector_wrapper_all_targets(float,
                                                                          mul,
                                                                          16)
                                            define_vector_wrapper_all_targets(
                                                float, div, 16)

    /* --- double arithmetic --- */
    define_vector_wrapper_all_targets(
        double, add, 2) define_vector_wrapper_all_targets(double, sub, 2)
        define_vector_wrapper_all_targets(
            double, mul, 2) define_vector_wrapper_all_targets(double, div, 2)

            define_vector_wrapper_all_targets(
                double, add, 4) define_vector_wrapper_all_targets(double, sub,
                                                                  4)
                define_vector_wrapper_all_targets(
                    double, mul, 4) define_vector_wrapper_all_targets(double,
                                                                      div, 4)

                    define_vector_wrapper_all_targets(
                        double, add,
                        8) define_vector_wrapper_all_targets(double, sub, 8)
                        define_vector_wrapper_all_targets(double, mul, 8)
                            define_vector_wrapper_all_targets(double, div, 8)

                                define_vector_wrapper_all_targets(double, add,
                                                                  16)
                                    define_vector_wrapper_all_targets(double,
                                                                      sub, 16)
                                        define_vector_wrapper_all_targets(
                                            double, mul, 16)
                                            define_vector_wrapper_all_targets(
                                                double, div, 16)

    /* --- float comparisons --- */
    define_vector_cmp_all_targets(float, 2)
        define_vector_cmp_all_targets(float, 4)
            define_vector_cmp_all_targets(float, 8)
                define_vector_cmp_all_targets(float, 16)

    /* --- double comparisons --- */
    define_vector_cmp_all_targets(double, 2)
        define_vector_cmp_all_targets(double, 4)
            define_vector_cmp_all_targets(double, 8)
                define_vector_cmp_all_targets(double, 16)

    /* --- float FMA --- */
    define_vector_fma_all_targets(float, 2)
        define_vector_fma_all_targets(float, 4)
            define_vector_fma_all_targets(float, 8)
                define_vector_fma_all_targets(float, 16)

    /* --- double FMA --- */
    define_vector_fma_all_targets(double, 2)
        define_vector_fma_all_targets(double, 4)
            define_vector_fma_all_targets(double, 8)
                define_vector_fma_all_targets(double, 16)

    /* =========================================================================
     * Dispatch table: maps a normalized feature string to function pointers.
     * Used by the LLVM pass (or a runtime resolver) to pick the right version.
     *
     * For each (precision, operation, size), the pass looks up:
     *   interflop_vector_dispatch[isa_level]._Nxprecisionop
     * =========================================================================
     */

    typedef enum {
      INTERFLOP_ISA_SSE2 = 0,
      INTERFLOP_ISA_AVX = 1,
      INTERFLOP_ISA_AVX2 = 2,
      INTERFLOP_ISA_AVX512 = 3,
      INTERFLOP_ISA_COUNT = 4,
    } interflop_isa_level_t;

/* Map a target-features string to an ISA level */
static inline interflop_isa_level_t
interflop_isa_from_features(const char *features) {
  /* features is the normalized "+avx512f" / "+avx" / "+sse2" etc. string */
  if (features && __builtin_strstr(features, "+avx512f"))
    return INTERFLOP_ISA_AVX512;
  if (features && __builtin_strstr(features, "+avx2"))
    return INTERFLOP_ISA_AVX2;
  if (features && __builtin_strstr(features, "+avx"))
    return INTERFLOP_ISA_AVX;
  return INTERFLOP_ISA_SSE2;
}

/*
 * Function pointer table for one (precision x size) arithmetic operation.
 * Indexed by interflop_isa_level_t.
 *
 * Example for float4 add:
 *   interflop_vec_f4add_table[INTERFLOP_ISA_AVX512] == _4xfloatadd_avx512
 */

#define DEFINE_DISPATCH_TABLE(precision, pshort, operation, opshort, size)     \
  typedef precision##size (*interflop_vec_##pshort##size##opshort##_fn)(       \
      const precision##size, const precision##size);                           \
  static const interflop_vec_##pshort##size##opshort##_fn                      \
      interflop_vec_##pshort##size##opshort##_table[INTERFLOP_ISA_COUNT] = {   \
          [INTERFLOP_ISA_SSE2] = _##size##x##precision##operation##_sse2,      \
          [INTERFLOP_ISA_AVX] = _##size##x##precision##operation##_avx,        \
          [INTERFLOP_ISA_AVX2] = _##size##x##precision##operation##_avx2,      \
          [INTERFLOP_ISA_AVX512] = _##size##x##precision##operation##_avx512,  \
  };

/* float */
DEFINE_DISPATCH_TABLE(float, f, add, add, 2)
DEFINE_DISPATCH_TABLE(float, f, add, add, 4)
DEFINE_DISPATCH_TABLE(float, f, add, add, 8)
DEFINE_DISPATCH_TABLE(float, f, add, add, 16)
DEFINE_DISPATCH_TABLE(float, f, sub, sub, 2)
DEFINE_DISPATCH_TABLE(float, f, sub, sub, 4)
DEFINE_DISPATCH_TABLE(float, f, sub, sub, 8)
DEFINE_DISPATCH_TABLE(float, f, sub, sub, 16)
DEFINE_DISPATCH_TABLE(float, f, mul, mul, 2)
DEFINE_DISPATCH_TABLE(float, f, mul, mul, 4)
DEFINE_DISPATCH_TABLE(float, f, mul, mul, 8)
DEFINE_DISPATCH_TABLE(float, f, mul, mul, 16)
DEFINE_DISPATCH_TABLE(float, f, div, div, 2)
DEFINE_DISPATCH_TABLE(float, f, div, div, 4)
DEFINE_DISPATCH_TABLE(float, f, div, div, 8)
DEFINE_DISPATCH_TABLE(float, f, div, div, 16)

/* double */
DEFINE_DISPATCH_TABLE(double, d, add, add, 2)
DEFINE_DISPATCH_TABLE(double, d, add, add, 4)
DEFINE_DISPATCH_TABLE(double, d, add, add, 8)
DEFINE_DISPATCH_TABLE(double, d, add, add, 16)
DEFINE_DISPATCH_TABLE(double, d, sub, sub, 2)
DEFINE_DISPATCH_TABLE(double, d, sub, sub, 4)
DEFINE_DISPATCH_TABLE(double, d, sub, sub, 8)
DEFINE_DISPATCH_TABLE(double, d, sub, sub, 16)
DEFINE_DISPATCH_TABLE(double, d, mul, mul, 2)
DEFINE_DISPATCH_TABLE(double, d, mul, mul, 4)
DEFINE_DISPATCH_TABLE(double, d, mul, mul, 8)
DEFINE_DISPATCH_TABLE(double, d, mul, mul, 16)
DEFINE_DISPATCH_TABLE(double, d, div, div, 2)
DEFINE_DISPATCH_TABLE(double, d, div, div, 4)
DEFINE_DISPATCH_TABLE(double, d, div, div, 8)
DEFINE_DISPATCH_TABLE(double, d, div, div, 16)
