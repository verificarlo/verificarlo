/*
 * interflop_vector_backend_arm.h
 *
 * Target-versioned vectorized wrappers for AArch64 (ARM64).
 *
 * ABI boundaries on AArch64:
 *   baseline : scalar FP in d0-d7 (64-bit)
 *   +neon    : SIMD in v0-v7 (128-bit fixed)     ← main SIMD ABI change
 *   +sve     : scalable Z registers (128–2048-bit) ← new register class
 *   +sme     : ZA matrix tile array               ← streaming mode
 *
 * Fixed-size GCC vectors (float4, float8, ...) map to:
 *   float2    (64-bit)  → baseline / NEON d-reg
 *   float4   (128-bit)  → NEON v-reg
 *   float8   (256-bit)  → 2x NEON or SVE (compiler decides)
 *   float16  (512-bit)  → 4x NEON or SVE
 *   double2  (128-bit)  → NEON v-reg
 *   double4  (256-bit)  → 2x NEON or SVE
 *   double8  (512-bit)  → SVE
 *   double16 (1024-bit) → SVE (requires 128-bit+ SVE implementation)
 *
 * Note: SVE vector length is implementation-defined (VLEN ≥ 128 bits).
 * Fixed-size GCC vectors work with SVE but the compiler may spill for
 * large sizes on narrow SVE implementations.
 */

#pragma once

#include <stdint.h>

#include "backends.h"
#include "interflop/interflop.h"

/* =========================================================================
 * Vector type definitions
 * ========================================================================= */

typedef float float2 __attribute__((vector_size(2 * sizeof(float))));
typedef float float4 __attribute__((vector_size(4 * sizeof(float))));
typedef float float8 __attribute__((vector_size(8 * sizeof(float))));
typedef float float16 __attribute__((vector_size(16 * sizeof(float))));

typedef double double2 __attribute__((vector_size(2 * sizeof(double))));
typedef double double4 __attribute__((vector_size(4 * sizeof(double))));
typedef double double8 __attribute__((vector_size(8 * sizeof(double))));
typedef double double16 __attribute__((vector_size(16 * sizeof(double))));

typedef int32_t int2 __attribute__((vector_size(2 * sizeof(int32_t))));
typedef int32_t int4 __attribute__((vector_size(4 * sizeof(int32_t))));
typedef int32_t int8 __attribute__((vector_size(8 * sizeof(int32_t))));
typedef int32_t int16 __attribute__((vector_size(16 * sizeof(int32_t))));

/* =========================================================================
 * ISA target attribute macros
 *
 * AArch64 target() strings understood by Clang/GCC:
 *   "neon"          → enables Advanced SIMD (NEON)
 *   "sve"           → enables SVE
 *   "sve2"          → enables SVE2 (implies SVE)
 *   "sme"           → enables SME (implies SVE2)
 *   "+sve2+sve2-bitperm+sve2-sha3+sve2-sm4+sve2-aes"  → SVE2 + all extensions
 *
 * "arch=armv8-a+sve" can also be used but target() is more portable.
 * ========================================================================= */

/* Baseline: no SIMD attribute — scalar FP only, vectors passed via pointer */
#define TARGET_ARM_BASELINE /* no attribute — use compiler default */

/* NEON: 128-bit Advanced SIMD, vectors in v0-v7 */
#define TARGET_ARM_NEON __attribute__((target("arch=armv8-a+simd")))

/* SVE: scalable vectors, Z0-Z31, P0-P15 */
#define TARGET_ARM_SVE __attribute__((target("arch=armv8-a+sve")))

/* SVE2: extends SVE (same register file, same ABI as SVE) */
#define TARGET_ARM_SVE2 __attribute__((target("arch=armv9-a+sve2")))

/* =========================================================================
 * Loop unroll hint
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
 * Arithmetic wrappers — one macro per (target, precision, operation, size)
 * ========================================================================= */

#define define_arm_vector_wrapper(TARGET_ATTR, TARGET_SUFFIX, precision,       \
                                  operation, size)                             \
  TARGET_ATTR                                                                  \
  precision##size _##size##x##precision##operation##_##TARGET_SUFFIX(          \
      const precision##size a, const precision##size b) {                      \
    precision##size c;                                                         \
    UNROLL(size)                                                               \
    for (int i = 0; i < (size); i++) {                                         \
      c[i] = backends[i].interflop_##precision##operation(a[i], b[i],          \
                                                          contexts[i]);        \
    }                                                                          \
    return c;                                                                  \
  }

#define define_arm_vector_wrapper_all_targets(precision, operation, size)      \
  define_arm_vector_wrapper(TARGET_ARM_BASELINE, baseline, precision,          \
                            operation, size)                                   \
      define_arm_vector_wrapper(TARGET_ARM_NEON, neon, precision, operation,   \
                                size)                                          \
          define_arm_vector_wrapper(TARGET_ARM_SVE, sve, precision, operation, \
                                    size)                                      \
              define_arm_vector_wrapper(TARGET_ARM_SVE2, sve2, precision,      \
                                        operation, size)

/* =========================================================================
 * Comparison wrappers
 * ========================================================================= */

#define define_arm_vector_cmp(TARGET_ATTR, TARGET_SUFFIX, precision, size)     \
  TARGET_ATTR                                                                  \
  int##size _##size##x##precision##cmp##_##TARGET_SUFFIX(                      \
      enum FCMP_PREDICATE p, precision##size a, precision##size b) {           \
    int##size c;                                                               \
    UNROLL(size)                                                               \
    for (int i = 0; i < (size); i++) {                                         \
      c[i] =                                                                   \
          backends[i].interflop_##precision##cmp(p, a[i], b[i], contexts[i]);  \
    }                                                                          \
    return c;                                                                  \
  }

#define define_arm_vector_cmp_all_targets(precision, size)                     \
  define_arm_vector_cmp(TARGET_ARM_BASELINE, baseline, precision, size)        \
      define_arm_vector_cmp(TARGET_ARM_NEON, neon, precision, size)            \
          define_arm_vector_cmp(TARGET_ARM_SVE, sve, precision, size)          \
              define_arm_vector_cmp(TARGET_ARM_SVE2, sve2, precision, size)

/* =========================================================================
 * FMA wrappers
 * ========================================================================= */

#define define_arm_vector_fma(TARGET_ATTR, TARGET_SUFFIX, precision, size)     \
  TARGET_ATTR                                                                  \
  precision##size _##size##x##precision##fma##_##TARGET_SUFFIX(                \
      const precision##size a, const precision##size b,                        \
      const precision##size c) {                                               \
    precision##size d;                                                         \
    UNROLL(size)                                                               \
    for (int i = 0; i < (size); i++) {                                         \
      d[i] = backends[i].interflop_##precision##fma(a[i], b[i], c[i],          \
                                                    contexts[i]);              \
    }                                                                          \
    return d;                                                                  \
  }

#define define_arm_vector_fma_all_targets(precision, size)                     \
  define_arm_vector_fma(TARGET_ARM_BASELINE, baseline, precision, size)        \
      define_arm_vector_fma(TARGET_ARM_NEON, neon, precision, size)            \
          define_arm_vector_fma(TARGET_ARM_SVE, sve, precision, size)          \
              define_arm_vector_fma(TARGET_ARM_SVE2, sve2, precision, size)

/* =========================================================================
 * Instantiate all variants
 * ========================================================================= */

/* float arithmetic */
define_arm_vector_wrapper_all_targets(
    float, add, 2) define_arm_vector_wrapper_all_targets(float, sub, 2)
    define_arm_vector_wrapper_all_targets(
        float, mul, 2) define_arm_vector_wrapper_all_targets(float, div, 2)

        define_arm_vector_wrapper_all_targets(
            float, add, 4) define_arm_vector_wrapper_all_targets(float, sub, 4)
            define_arm_vector_wrapper_all_targets(
                float, mul, 4) define_arm_vector_wrapper_all_targets(float, div,
                                                                     4)

                define_arm_vector_wrapper_all_targets(
                    float, add, 8) define_arm_vector_wrapper_all_targets(float,
                                                                         sub, 8)
                    define_arm_vector_wrapper_all_targets(float, mul, 8)
                        define_arm_vector_wrapper_all_targets(float, div, 8)

                            define_arm_vector_wrapper_all_targets(float, add,
                                                                  16)
                                define_arm_vector_wrapper_all_targets(float,
                                                                      sub, 16)
                                    define_arm_vector_wrapper_all_targets(float,
                                                                          mul,
                                                                          16)
                                        define_arm_vector_wrapper_all_targets(
                                            float, div, 16)

    /* double arithmetic */
    define_arm_vector_wrapper_all_targets(
        double, add, 2) define_arm_vector_wrapper_all_targets(double, sub, 2)
        define_arm_vector_wrapper_all_targets(
            double, mul, 2) define_arm_vector_wrapper_all_targets(double, div,
                                                                  2)

            define_arm_vector_wrapper_all_targets(
                double, add, 4) define_arm_vector_wrapper_all_targets(double,
                                                                      sub, 4)
                define_arm_vector_wrapper_all_targets(
                    double, mul,
                    4) define_arm_vector_wrapper_all_targets(double, div, 4)

                    define_arm_vector_wrapper_all_targets(
                        double, add,
                        8) define_arm_vector_wrapper_all_targets(double, sub, 8)
                        define_arm_vector_wrapper_all_targets(
                            double, mul,
                            8) define_arm_vector_wrapper_all_targets(double,
                                                                     div, 8)

                            define_arm_vector_wrapper_all_targets(double, add,
                                                                  16)
                                define_arm_vector_wrapper_all_targets(double,
                                                                      sub, 16)
                                    define_arm_vector_wrapper_all_targets(
                                        double, mul, 16)
                                        define_arm_vector_wrapper_all_targets(
                                            double, div, 16)

    /* float comparisons */
    define_arm_vector_cmp_all_targets(float, 2)
        define_arm_vector_cmp_all_targets(float, 4)
            define_arm_vector_cmp_all_targets(float, 8)
                define_arm_vector_cmp_all_targets(float, 16)

    /* double comparisons */
    define_arm_vector_cmp_all_targets(double, 2)
        define_arm_vector_cmp_all_targets(double, 4)
            define_arm_vector_cmp_all_targets(double, 8)
                define_arm_vector_cmp_all_targets(double, 16)

    /* float FMA */
    define_arm_vector_fma_all_targets(float, 2)
        define_arm_vector_fma_all_targets(float, 4)
            define_arm_vector_fma_all_targets(float, 8)
                define_arm_vector_fma_all_targets(float, 16)

    /* double FMA */
    define_arm_vector_fma_all_targets(double, 2)
        define_arm_vector_fma_all_targets(double, 4)
            define_arm_vector_fma_all_targets(double, 8)
                define_arm_vector_fma_all_targets(double, 16)

    /* =========================================================================
     * ISA level enum and dispatch table
     * =========================================================================
     */

    typedef enum {
      INTERFLOP_ARM_ISA_BASELINE = 0,
      INTERFLOP_ARM_ISA_NEON = 1,
      INTERFLOP_ARM_ISA_SVE = 2,
      INTERFLOP_ARM_ISA_SVE2 = 3,
      INTERFLOP_ARM_ISA_COUNT = 4,
    } interflop_arm_isa_level_t;

static inline interflop_arm_isa_level_t
interflop_arm_isa_from_features(const char *features) {
  if (features && __builtin_strstr(features, "+sve2"))
    return INTERFLOP_ARM_ISA_SVE2;
  if (features && __builtin_strstr(features, "+sve"))
    return INTERFLOP_ARM_ISA_SVE;
  if (features && __builtin_strstr(features, "+neon"))
    return INTERFLOP_ARM_ISA_NEON;
  return INTERFLOP_ARM_ISA_BASELINE;
}

#define DEFINE_ARM_DISPATCH_TABLE(precision, pshort, operation, opshort, size) \
  typedef precision##size (*interflop_arm_vec_##pshort##size##opshort##_fn)(   \
      const precision##size, const precision##size);                           \
  static const interflop_arm_vec_##pshort##size##opshort##_fn                  \
      interflop_arm_vec_##pshort##size##opshort##_table                        \
          [INTERFLOP_ARM_ISA_COUNT] = {                                        \
              [INTERFLOP_ARM_ISA_BASELINE] =                                   \
                  _##size##x##precision##operation##_baseline,                 \
              [INTERFLOP_ARM_ISA_NEON] =                                       \
                  _##size##x##precision##operation##_neon,                     \
              [INTERFLOP_ARM_ISA_SVE] =                                        \
                  _##size##x##precision##operation##_sve,                      \
              [INTERFLOP_ARM_ISA_SVE2] =                                       \
                  _##size##x##precision##operation##_sve2,                     \
  };

/* float */
DEFINE_ARM_DISPATCH_TABLE(float, f, add, add, 2)
DEFINE_ARM_DISPATCH_TABLE(float, f, add, add, 4)
DEFINE_ARM_DISPATCH_TABLE(float, f, add, add, 8)
DEFINE_ARM_DISPATCH_TABLE(float, f, add, add, 16)
DEFINE_ARM_DISPATCH_TABLE(float, f, sub, sub, 2)
DEFINE_ARM_DISPATCH_TABLE(float, f, sub, sub, 4)
DEFINE_ARM_DISPATCH_TABLE(float, f, sub, sub, 8)
DEFINE_ARM_DISPATCH_TABLE(float, f, sub, sub, 16)
DEFINE_ARM_DISPATCH_TABLE(float, f, mul, mul, 2)
DEFINE_ARM_DISPATCH_TABLE(float, f, mul, mul, 4)
DEFINE_ARM_DISPATCH_TABLE(float, f, mul, mul, 8)
DEFINE_ARM_DISPATCH_TABLE(float, f, mul, mul, 16)
DEFINE_ARM_DISPATCH_TABLE(float, f, div, div, 2)
DEFINE_ARM_DISPATCH_TABLE(float, f, div, div, 4)
DEFINE_ARM_DISPATCH_TABLE(float, f, div, div, 8)
DEFINE_ARM_DISPATCH_TABLE(float, f, div, div, 16)

/* double */
DEFINE_ARM_DISPATCH_TABLE(double, d, add, add, 2)
DEFINE_ARM_DISPATCH_TABLE(double, d, add, add, 4)
DEFINE_ARM_DISPATCH_TABLE(double, d, add, add, 8)
DEFINE_ARM_DISPATCH_TABLE(double, d, add, add, 16)
DEFINE_ARM_DISPATCH_TABLE(double, d, sub, sub, 2)
DEFINE_ARM_DISPATCH_TABLE(double, d, sub, sub, 4)
DEFINE_ARM_DISPATCH_TABLE(double, d, sub, sub, 8)
DEFINE_ARM_DISPATCH_TABLE(double, d, sub, sub, 16)
DEFINE_ARM_DISPATCH_TABLE(double, d, mul, mul, 2)
DEFINE_ARM_DISPATCH_TABLE(double, d, mul, mul, 4)
DEFINE_ARM_DISPATCH_TABLE(double, d, mul, mul, 8)
DEFINE_ARM_DISPATCH_TABLE(double, d, mul, mul, 16)
DEFINE_ARM_DISPATCH_TABLE(double, d, div, div, 2)
DEFINE_ARM_DISPATCH_TABLE(double, d, div, div, 4)
DEFINE_ARM_DISPATCH_TABLE(double, d, div, div, 8)
DEFINE_ARM_DISPATCH_TABLE(double, d, div, div, 16)
