#ifndef _VPREC_TOOLS_VECTOR_H_
#define _VPREC_TOOLS_VECTOR_H_

/**
 * Macro which define vector function to round binary32 normal
 */
#define define_round_binary32_normal_vector(size)                              \
  inline void round_binary32_normal_##size##x(float##size *x,                  \
                                              int##size precision) {           \
    /* build 1/2 ulp and add it  before truncation for faithfull rounding */   \
                                                                               \
    /* generate a mask to erase the last 23-VPRECLIB_PREC bits, in other       \
       words, there remain VPRECLIB_PREC bits in the mantissa */               \
    const uint32_##size##x mask =                                              \
        ((uint32_##size##x)0xFFFFFFFF                                          \
         << ((uint32_##size##x)FLOAT_PMAN_SIZE - precision));                  \
                                                                               \
    /* position to the end of the target prec-1 */                             \
    const uint32_##size##x target_position =                                   \
        (uint32_##size##x)FLOAT_PMAN_SIZE - precision - (uint32_##size##x)1;   \
                                                                               \
    binary32_##size##x b32x = {.f32 = *x};                                     \
    b32x.ieee.mantissa = (uint32_##size##x)0;                                  \
    binary32_##size##x half_ulp = {.f32 = *x};                                 \
    half_ulp.ieee.mantissa =                                                   \
        (uint32_##size##x)((uint32_##size##x)1 << target_position);            \
                                                                               \
    b32x.f32 = *x + (half_ulp.f32 - b32x.f32);                                 \
    b32x.u32 &= mask;                                                          \
                                                                               \
    *x = b32x.f32;                                                             \
  }

/* Using above macro */
define_round_binary32_normal_vector(2);
define_round_binary32_normal_vector(4);
define_round_binary32_normal_vector(8);
define_round_binary32_normal_vector(16);

/**
 * Macro which define vector function to round binary64 normal
 */
#define define_round_binary64_normal_vector(size)                              \
  inline void round_binary64_normal_##size##x(double##size *x,                 \
                                              int64_##size##x precision) {     \
    /* build 1/2 ulp and add it  before truncation for faithfull rounding */   \
                                                                               \
    /* generate a mask to erase the last 23-VPRECLIB_PREC bits, in other       \
       words, there remain VPRECLIB_PREC bits in the mantissa */               \
    const uint64_##size##x mask =                                              \
        ((uint64_##size##x)0xFFFFFFFFFFFFFFFF                                  \
         << ((uint64_##size##x)DOUBLE_PMAN_SIZE - precision));                 \
                                                                               \
    /* position to the end of the target prec-1 */                             \
    const uint64_##size##x target_position =                                   \
        (uint64_##size##x)DOUBLE_PMAN_SIZE - precision - (uint64_##size##x)1;  \
                                                                               \
    binary64_##size##x b64x = {.f64 = *x};                                     \
    b64x.ieee.mantissa = (uint64_##size##x)0;                                  \
    binary64_##size##x half_ulp = {.f64 = *x};                                 \
    half_ulp.ieee.mantissa =                                                   \
        (uint64_##size##x)((uint64_##size##x)1ULL << target_position);         \
                                                                               \
    b64x.f64 = *x + (half_ulp.f64 - b64x.f64);                                 \
    b64x.u64 &= mask;                                                          \
                                                                               \
    *x = b64x.f64;                                                             \
  }

/* Using above macro */
define_round_binary64_normal_vector(2);
define_round_binary64_normal_vector(4);
define_round_binary64_normal_vector(8);
define_round_binary64_normal_vector(16);

#endif // _VPREC_TOOLS_VECTOR_H_
