/*
 * interflop_vector_backend_amd.h
 *
 * Target-versioned vectorized wrappers for AMD x86-64 (Zen architecture).
 *
 * ABI boundaries are identical to Intel x86-64 (same ISA):
 *   +sse2    → XMM registers (128-bit)   [x86-64 baseline, always present]
 *   +avx     → YMM registers (256-bit)   [Bulldozer/Jaguar and later]
 *   +avx512f → ZMM registers (512-bit)   [Zen 4 (znver4) and later]
 *
 * AMD Zen microarchitecture → ISA support:
 *   znver1  (Zen 1, 2017): SSE2 + AVX2, NO AVX-512
 *   znver2  (Zen 2, 2019): SSE2 + AVX2, NO AVX-512
 *   znver3  (Zen 3, 2020): SSE2 + AVX2, NO AVX-512
 *   znver4  (Zen 4, 2022): SSE2 + AVX2 + AVX-512
 *   znver5  (Zen 5, 2024): SSE2 + AVX2 + AVX-512 (512-bit FP units)
 *
 * Key AMD performance notes:
 *   - Zen 1/2/3 have 256-bit AVX execution units (split-256 microcode)
 *   - Zen 4's AVX-512 uses 256-bit units internally (2 uops for 512-bit ops)
 *   - Zen 5 has native 512-bit execution units
 *   → Prefer AVX2 over AVX-512 on znver4; prefer AVX-512 on znver5
 *
 * The ISA suffix mapping for this backend:
 *   _sse2    → tuned for znver1/znver2/znver3 without AVX
 *   _avx2    → tuned for znver1/znver2/znver3 (AVX2, no 512)
 *   _avx512  → tuned for znver4 (cost-effective 512)
 *   _avx512z5→ tuned for znver5 (native 512-bit units)
 */

#pragma once

#include <stdint.h>

#include "backends.h"
#include "interflop/interflop.h"
#include "vector_ext.h"

/* =========================================================================
 * AMD ISA target attribute macros
 *
 * We combine the ABI-defining feature (avx/avx512f) with the AMD CPU name
 * so the compiler can apply Zen-specific scheduling and cost models.
 *
 * Clang accepts: __attribute__((target("cpu=znver4,avx512f")))
 * GCC  accepts: __attribute__((target("arch=znver4")))  [implies avx512f on
 * znver4]
 *
 * We use the feature-string form for maximum portability across compilers.
 * ========================================================================= */

/*
 * SSE2 only — targets Zen 1/2/3 code paths where AVX was not enabled
 * (e.g., libraries compiled without -mavx for compatibility)
 */
#define TARGET_AMD_SSE2 __attribute__((target("sse2,no-avx")))

/*
 * AVX2 + FMA — optimal for Zen 1/2/3 (native 256-bit execution units)
 * Also used as the AVX2 path on Zen 4/5.
 */
#define TARGET_AMD_AVX2 __attribute__((target("avx2,fma,bmi,bmi2")))

/*
 * AVX-512 tuned for Zen 4 — 256-bit internal units, prefer 256-bit zmm ops.
 * Use prefer-256-bit to hint the compiler to use 256-bit ymm over 512-bit zmm
 * where equivalent, avoiding the frequency throttling penalty on znver4.
 */
#define TARGET_AMD_AVX512_ZEN4                                                 \
  __attribute__((target("avx512f,avx512bw,avx512vl,avx512dq,avx512vnni")))

/*
 * AVX-512 tuned for Zen 5 — native 512-bit execution units.
 * No prefer-256-bit: full-width 512-bit ops are first-class on znver5.
 */
#define TARGET_AMD_AVX512_ZEN5                                                 \
  __attribute__((                                                              \
      target("avx512f,avx512bw,avx512vl,avx512dq,avx512vnni,avx512bf16")))

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
 * Arithmetic wrapper macro
 * ========================================================================= */

#define define_amd_vector_wrapper(TARGET_ATTR, TARGET_SUFFIX, precision,       \
                                  operation, size)                             \
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

#define define_amd_vector_wrapper_all_targets(precision, operation, size)      \
  define_amd_vector_wrapper(TARGET_AMD_SSE2, sse2, precision, operation, size) \
      define_amd_vector_wrapper(TARGET_AMD_AVX2, avx2, precision, operation,   \
                                size)                                          \
          define_amd_vector_wrapper(TARGET_AMD_AVX512_ZEN4, avx512_zen4,       \
                                    precision, operation, size)                \
              define_amd_vector_wrapper(TARGET_AMD_AVX512_ZEN5, avx512_zen5,   \
                                        precision, operation, size)

/* =========================================================================
 * Comparison wrapper macro
 * ========================================================================= */

#define define_amd_vector_cmp(TARGET_ATTR, TARGET_SUFFIX, precision, size)     \
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

#define define_amd_vector_cmp_all_targets(precision, size)                     \
  define_amd_vector_cmp(TARGET_AMD_SSE2, sse2, precision, size)                \
      define_amd_vector_cmp(TARGET_AMD_AVX2, avx2, precision, size)            \
          define_amd_vector_cmp(TARGET_AMD_AVX512_ZEN4, avx512_zen4,           \
                                precision, size)                               \
              define_amd_vector_cmp(TARGET_AMD_AVX512_ZEN5, avx512_zen5,       \
                                    precision, size)

/* =========================================================================
 * FMA wrapper macro
 * ========================================================================= */

#define define_amd_vector_fma(TARGET_ATTR, TARGET_SUFFIX, precision, size)     \
  TARGET_ATTR                                                                  \
  precision##size _##size##x##precision##fma##_##TARGET_SUFFIX(                \
      const precision##size a, const precision##size b,                        \
      const precision##size c) {                                               \
    typedef union {                                                            \
      precision##size v;                                                       \
      precision a[size];                                                       \
    } fvec;                                                                    \
    fvec au = {.v = a}, bu = {.v = b}, cu = {.v = c}, du;                      \
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

#define define_amd_vector_fma_all_targets(precision, size)                     \
  define_amd_vector_fma(TARGET_AMD_SSE2, sse2, precision, size)                \
      define_amd_vector_fma(TARGET_AMD_AVX2, avx2, precision, size)            \
          define_amd_vector_fma(TARGET_AMD_AVX512_ZEN4, avx512_zen4,           \
                                precision, size)                               \
              define_amd_vector_fma(TARGET_AMD_AVX512_ZEN5, avx512_zen5,       \
                                    precision, size)

/* =========================================================================
 * Instantiate all variants
 * ========================================================================= */

/* float arithmetic */
define_amd_vector_wrapper_all_targets(
    float, add, 2) define_amd_vector_wrapper_all_targets(float, sub, 2)
    define_amd_vector_wrapper_all_targets(
        float, mul, 2) define_amd_vector_wrapper_all_targets(float, div, 2)

        define_amd_vector_wrapper_all_targets(
            float, add, 4) define_amd_vector_wrapper_all_targets(float, sub, 4)
            define_amd_vector_wrapper_all_targets(
                float, mul, 4) define_amd_vector_wrapper_all_targets(float, div,
                                                                     4)

                define_amd_vector_wrapper_all_targets(
                    float, add, 8) define_amd_vector_wrapper_all_targets(float,
                                                                         sub, 8)
                    define_amd_vector_wrapper_all_targets(float, mul, 8)
                        define_amd_vector_wrapper_all_targets(float, div, 8)

                            define_amd_vector_wrapper_all_targets(float, add,
                                                                  16)
                                define_amd_vector_wrapper_all_targets(float,
                                                                      sub, 16)
                                    define_amd_vector_wrapper_all_targets(float,
                                                                          mul,
                                                                          16)
                                        define_amd_vector_wrapper_all_targets(
                                            float, div, 16)

    /* double arithmetic */
    define_amd_vector_wrapper_all_targets(
        double, add, 2) define_amd_vector_wrapper_all_targets(double, sub, 2)
        define_amd_vector_wrapper_all_targets(
            double, mul, 2) define_amd_vector_wrapper_all_targets(double, div,
                                                                  2)

            define_amd_vector_wrapper_all_targets(
                double, add, 4) define_amd_vector_wrapper_all_targets(double,
                                                                      sub, 4)
                define_amd_vector_wrapper_all_targets(
                    double, mul,
                    4) define_amd_vector_wrapper_all_targets(double, div, 4)

                    define_amd_vector_wrapper_all_targets(
                        double, add,
                        8) define_amd_vector_wrapper_all_targets(double, sub, 8)
                        define_amd_vector_wrapper_all_targets(
                            double, mul,
                            8) define_amd_vector_wrapper_all_targets(double,
                                                                     div, 8)

                            define_amd_vector_wrapper_all_targets(double, add,
                                                                  16)
                                define_amd_vector_wrapper_all_targets(double,
                                                                      sub, 16)
                                    define_amd_vector_wrapper_all_targets(
                                        double, mul, 16)
                                        define_amd_vector_wrapper_all_targets(
                                            double, div, 16)

    /* float comparisons */
    define_amd_vector_cmp_all_targets(float, 2)
        define_amd_vector_cmp_all_targets(float, 4)
            define_amd_vector_cmp_all_targets(float, 8)
                define_amd_vector_cmp_all_targets(float, 16)

    /* double comparisons */
    define_amd_vector_cmp_all_targets(double, 2)
        define_amd_vector_cmp_all_targets(double, 4)
            define_amd_vector_cmp_all_targets(double, 8)
                define_amd_vector_cmp_all_targets(double, 16)

    /* float FMA */
    define_amd_vector_fma_all_targets(float, 2)
        define_amd_vector_fma_all_targets(float, 4)
            define_amd_vector_fma_all_targets(float, 8)
                define_amd_vector_fma_all_targets(float, 16)

    /* double FMA */
    define_amd_vector_fma_all_targets(double, 2)
        define_amd_vector_fma_all_targets(double, 4)
            define_amd_vector_fma_all_targets(double, 8)
                define_amd_vector_fma_all_targets(double, 16)

    /* =========================================================================
     * ISA level enum + feature detection
     * =========================================================================
     */

    typedef enum {
      INTERFLOP_AMD_ISA_SSE2 = 0,        /* znver1/2/3 without AVX */
      INTERFLOP_AMD_ISA_AVX2 = 1,        /* znver1/2/3 with AVX2   */
      INTERFLOP_AMD_ISA_AVX512_ZEN4 = 2, /* znver4                  */
      INTERFLOP_AMD_ISA_AVX512_ZEN5 = 3, /* znver5                  */
      INTERFLOP_AMD_ISA_COUNT = 4,
    } interflop_amd_isa_level_t;

/*
 * Detect AMD ISA level from target-features string.
 *
 * Distinguishing Zen 4 from Zen 5 from features alone is hard;
 * use the cpu name ("znver5") if available, otherwise fall back to
 * avx512f presence (maps to zen4 conservatively).
 */
static inline interflop_amd_isa_level_t
interflop_amd_isa_from_features(const char *features, const char *cpu) {
  /* Check CPU name first for Zen generation */
  if (cpu) {
    if (__builtin_strstr(cpu, "znver5"))
      return INTERFLOP_AMD_ISA_AVX512_ZEN5;
    if (__builtin_strstr(cpu, "znver4"))
      return INTERFLOP_AMD_ISA_AVX512_ZEN4;
  }
  /* Fall back to feature string */
  if (features && __builtin_strstr(features, "+avx512f"))
    return INTERFLOP_AMD_ISA_AVX512_ZEN4;
  if (features && __builtin_strstr(features, "+avx"))
    return INTERFLOP_AMD_ISA_AVX2;
  return INTERFLOP_AMD_ISA_SSE2;
}

/* =========================================================================
 * Dispatch tables
 * ========================================================================= */

#define DEFINE_AMD_DISPATCH_TABLE(precision, pshort, operation, opshort, size) \
  typedef precision##size (*interflop_amd_vec_##pshort##size##opshort##_fn)(   \
      const precision##size, const precision##size);                           \
  static const interflop_amd_vec_##pshort##size##opshort##_fn                  \
      interflop_amd_vec_##pshort##size##opshort##_table                        \
          [INTERFLOP_AMD_ISA_COUNT] = {                                        \
              [INTERFLOP_AMD_ISA_SSE2] =                                       \
                  _##size##x##precision##operation##_sse2,                     \
              [INTERFLOP_AMD_ISA_AVX2] =                                       \
                  _##size##x##precision##operation##_avx2,                     \
              [INTERFLOP_AMD_ISA_AVX512_ZEN4] =                                \
                  _##size##x##precision##operation##_avx512_zen4,              \
              [INTERFLOP_AMD_ISA_AVX512_ZEN5] =                                \
                  _##size##x##precision##operation##_avx512_zen5,              \
  };

/* float */
DEFINE_AMD_DISPATCH_TABLE(float, f, add, add, 2)
DEFINE_AMD_DISPATCH_TABLE(float, f, add, add, 4)
DEFINE_AMD_DISPATCH_TABLE(float, f, add, add, 8)
DEFINE_AMD_DISPATCH_TABLE(float, f, add, add, 16)
DEFINE_AMD_DISPATCH_TABLE(float, f, sub, sub, 2)
DEFINE_AMD_DISPATCH_TABLE(float, f, sub, sub, 4)
DEFINE_AMD_DISPATCH_TABLE(float, f, sub, sub, 8)
DEFINE_AMD_DISPATCH_TABLE(float, f, sub, sub, 16)
DEFINE_AMD_DISPATCH_TABLE(float, f, mul, mul, 2)
DEFINE_AMD_DISPATCH_TABLE(float, f, mul, mul, 4)
DEFINE_AMD_DISPATCH_TABLE(float, f, mul, mul, 8)
DEFINE_AMD_DISPATCH_TABLE(float, f, mul, mul, 16)
DEFINE_AMD_DISPATCH_TABLE(float, f, div, div, 2)
DEFINE_AMD_DISPATCH_TABLE(float, f, div, div, 4)
DEFINE_AMD_DISPATCH_TABLE(float, f, div, div, 8)
DEFINE_AMD_DISPATCH_TABLE(float, f, div, div, 16)

/* double */
DEFINE_AMD_DISPATCH_TABLE(double, d, add, add, 2)
DEFINE_AMD_DISPATCH_TABLE(double, d, add, add, 4)
DEFINE_AMD_DISPATCH_TABLE(double, d, add, add, 8)
DEFINE_AMD_DISPATCH_TABLE(double, d, add, add, 16)
DEFINE_AMD_DISPATCH_TABLE(double, d, sub, sub, 2)
DEFINE_AMD_DISPATCH_TABLE(double, d, sub, sub, 4)
DEFINE_AMD_DISPATCH_TABLE(double, d, sub, sub, 8)
DEFINE_AMD_DISPATCH_TABLE(double, d, sub, sub, 16)
DEFINE_AMD_DISPATCH_TABLE(double, d, mul, mul, 2)
DEFINE_AMD_DISPATCH_TABLE(double, d, mul, mul, 4)
DEFINE_AMD_DISPATCH_TABLE(double, d, mul, mul, 8)
DEFINE_AMD_DISPATCH_TABLE(double, d, mul, mul, 16)
DEFINE_AMD_DISPATCH_TABLE(double, d, div, div, 2)
DEFINE_AMD_DISPATCH_TABLE(double, d, div, div, 4)
DEFINE_AMD_DISPATCH_TABLE(double, d, div, div, 8)
DEFINE_AMD_DISPATCH_TABLE(double, d, div, div, 16)
