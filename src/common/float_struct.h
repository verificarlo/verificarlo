#ifndef __FLOAT_STRUCT_H_
#define __FLOAT_STRUCT_H_

#include "float_const.h"
#include <stdint.h>
#include <stdlib.h>

/* import from <quadmath-imp.h> */

/* Frankly, if you have __float128, you have 64-bit integers, right?  */
#ifndef UINT64_C
#error "No way!"
#endif

/* Main union type we use to manipulate the floating-point type.  */

typedef union {
  __float128 f128;
  __uint128_t u128;

  /* Generic fields */
  __float128 type;
  __uint128_t u;

  struct
#ifdef __MINGW32__
      /* On mingw targets the ms-bitfields option is active by default.
         Therefore enforce gnu-bitfield style.  */
      __attribute__((gcc_struct))
#endif
  {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    unsigned sign : QUAD_SIGN_SIZE;
    unsigned exponent : QUAD_EXP_SIZE;
    uint64_t mant_high : QUAD_HX_PMAN_SIZE;
    uint64_t mant_low : QUAD_LX_PMAN_SIZE;
#else
    uint64_t mant_low : QUAD_LX_PMAN_SIZE;
    uint64_t mant_high : QUAD_HX_PMAN_SIZE;
    unsigned exponent : QUAD_EXP_SIZE;
    unsigned sign : QUAD_SIGN_SIZE;
#endif
  } ieee;

  struct
#ifdef __MINGW32__
      /* On mingw targets the ms-bitfields option is active by default.
         Therefore enforce gnu-bitfield style.  */
      __attribute__((gcc_struct))
#endif
  {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    __uint128_t sign : QUAD_SIGN_SIZE;
    __uint128_t exponent : QUAD_EXP_SIZE;
    __uint128_t mantissa : QUAD_PMAN_SIZE;
#else
    __uint128_t mantissa : QUAD_PMAN_SIZE;
    __uint128_t exponent : QUAD_EXP_SIZE;
    __uint128_t sign : QUAD_SIGN_SIZE;
#endif
  } ieee128;

  struct {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    uint64_t high;
    uint64_t low;
#else
    uint64_t low;
    uint64_t high;
#endif
  } words64;

  struct {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    uint32_t w0;
    uint32_t w1;
    uint32_t w2;
    uint32_t w3;
#else
    uint32_t w3;
    uint32_t w2;
    uint32_t w1;
    uint32_t w0;
#endif
  } words32;

  struct
#ifdef __MINGW32__
      /* Make sure we are using gnu-style bitfield handling.  */
      __attribute__((gcc_struct))
#endif
  {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    uint64_t sign : QUAD_SIGN_SIZE;
    uint64_t exponent : QUAD_EXP_SIZE;
    uint64_t quiet_nan : QUAD_QUIET_NAN_SIZE;
    uint64_t mant_high : QUAD_HX_PMAN_QNAN_SIZE;
    uint64_t mant_low : QUAD_LX_PMAN_QNAN_SIZE;
#else
    uint64_t mant_low : QUAD_LX_PMAN_QNAN_SIZE;
    uint64_t mant_high : QUAD_HX_PMAN_QNAN_SIZE;
    uint64_t quiet_nan : QUAD_QUIET_NAN_SIZE;
    uint64_t exponent : QUAD_EXP_SIZE;
    uint64_t sign : QUAD_SIGN_SIZE;
#endif
  } nan;

} binary128;

typedef union {

  double f64;
  uint64_t u64;
  int64_t s64;
  uint32_t u32[2];

  /* Generic fields */
  double type;
  uint64_t u;

  struct {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    uint64_t sign : DOUBLE_SIGN_SIZE;
    uint64_t exponent : DOUBLE_EXP_SIZE;
    uint64_t mantissa : DOUBLE_PMAN_SIZE;
#endif
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    uint64_t mantissa : DOUBLE_PMAN_SIZE;
    uint64_t exponent : DOUBLE_EXP_SIZE;
    uint64_t sign : DOUBLE_SIGN_SIZE;
#endif
  } ieee;

  struct {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    uint32_t sign : DOUBLE_SIGN_SIZE;
    uint32_t exponent : DOUBLE_EXP_SIZE;
    uint32_t mantissa_high : DOUBLE_PMAN_HIGH_SIZE;
    uint32_t mantissa_low : DOUBLE_PMAN_LOW_SIZE;
#endif
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#if __FLOAT_WORD_ORDER__ == __ORDER_BIG_ENDIAN__
    uint32_t mantissa_high : DOUBLE_PMAN_HIGH_SIZE;
    uint32_t exponent : DOUBLE_EXP_SIZE;
    uint32_t sign : DOUBLE_SIGN_SIZE;
    uint32_t mantissa_low : DOUBLE_PMAN_LOW_SIZE;
#else
    uint32_t mantissa_low : DOUBLE_PMAN_LOW_SIZE;
    uint32_t mantissa_high : DOUBLE_PMAN_HIGH_SIZE;
    uint32_t exponent : DOUBLE_EXP_SIZE;
    uint32_t sign : DOUBLE_SIGN_SIZE;
#endif
#endif
  } ieee32;

} binary64;

typedef union {

  float f32;
  uint32_t u32;
  int32_t s32;

  /* Generic fields */
  float type;
  uint32_t u;

  struct {
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    uint32_t sign : FLOAT_SIGN_SIZE;
    uint32_t exponent : FLOAT_EXP_SIZE;
    uint32_t mantissa : FLOAT_PMAN_SIZE;
#endif
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    uint32_t mantissa : FLOAT_PMAN_SIZE;
    uint32_t exponent : FLOAT_EXP_SIZE;
    uint32_t sign : FLOAT_SIGN_SIZE;
#endif
  } ieee;

} binary32;

#define QUADFP_NAN 0
#define QUADFP_INFINITE 1
#define QUADFP_ZERO 2
#define QUADFP_SUBNORMAL 3
#define QUADFP_NORMAL 4
#define fpclassifyq(x)                                                         \
  __builtin_fpclassify(QUADFP_NAN, QUADFP_INFINITE, QUADFP_NORMAL,             \
                       QUADFP_SUBNORMAL, QUADFP_ZERO, x)

#endif /* __FLOAT_STRUCT_H_ */
