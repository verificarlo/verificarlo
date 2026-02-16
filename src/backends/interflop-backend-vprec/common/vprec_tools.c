/*****************************************************************************\
 *                                                                           *\
 *  This file is part of the Verificarlo project,                            *\
 *  under the Apache License v2.0 with LLVM Exceptions.                      *\
 *  SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception.                 *\
 *  See https://llvm.org/LICENSE.txt for license information.                *\
 *                                                                           *\
 *                                                                           *\
 *  Copyright (c) 2015                                                       *\
 *     Universite de Versailles St-Quentin-en-Yvelines                       *\
 *     CMLA, Ecole Normale Superieure de Cachan                              *\
 *                                                                           *\
 *  Copyright (c) 2018                                                       *\
 *     Universite de Versailles St-Quentin-en-Yvelines                       *\
 *                                                                           *\
 *  Copyright (c) 2019-2026                                                  *\
 *     Verificarlo Contributors                                              *\
 *                                                                           *\
 ****************************************************************************/
#include <math.h>
#include <stdint.h>

#include "interflop/common/float_const.h"
#include "interflop/common/float_struct.h"
#include "interflop/iostream/logger.h"
#include "vprec_tools.h"

/**
 * print the binary128 number in hexadecimal format
 */
void print_binary128(const binary128 b128_x) {
  char buf[256];
  quadmath_snprintf(buf, sizeof(buf), "%+.28Qa", b128_x.f128);
  logger_debug("%s\n", buf);
}

/* check if we need to add .5 ulp to have the rounding to nearest with ties to
 * even */
inline static int check_if_binary32_needs_rounding(binary32 b32x,
                                                   int precision) {
  const uint32_t one = 1;

  // Create a mask to isolate the bits that will be rounded.
  const uint32_t trailing_bits_mask =
      (one << (FLOAT_PMAN_SIZE - precision)) - 1;
  const uint32_t trailing_bits = b32x.ieee.mantissa & trailing_bits_mask;

  // Calculate the bit that will be rounded
  const uint32_t bit_to_round_offset = (FLOAT_PMAN_SIZE - precision);
  const uint32_t bit_to_round = (b32x.ieee.mantissa >> bit_to_round_offset) & 1;

  // Calculate the halfway point for rounding.
  const uint32_t halfway_point = one << (FLOAT_PMAN_SIZE - precision - 1);

  // If the trailing bits are greater than the halfway point, or exactly equal
  // to it and the bit-to-round is 1 (for tie-breaking), round up by adding
  // half_ulp to x.
  return (trailing_bits > halfway_point) ||
         ((trailing_bits == halfway_point) && (bit_to_round == 1));
}

/**
 * round the mantissa of 'x' on the precision specified by 'precision'
 * this function does not check that 'precision' is within the correct
 * range; the user is responsible for ensuring that 'precision'
 * satisfies this requirement
 *   x: the binary32 number to round
 *   precision: the new virtual precision; 0<=precision<=FLOAT_PMAN_SIZE
 *   retval x rounded on the specified precision
 */
inline float round_binary32_normal(float x, int precision) {
  /* build 1/2 ulp and add it before truncation for rounding to nearest with
   * ties to even */

  /* generate a mask to erase the last 23-VPRECLIB_PREC bits, in other
     words, there remain VPRECLIB_PREC bits in the mantissa */
  const uint32_t mask = 0xFFFFFFFF << (FLOAT_PMAN_SIZE - precision);

  binary32 b32x = {.f32 = x};
  int exp_hulp = b32x.ieee.exponent - precision - 1;
  binary32 half_ulp;
  half_ulp.ieee.sign = x < 0;
  half_ulp.ieee.exponent = exp_hulp;
  half_ulp.ieee.mantissa = 0;

  /* Handle denormal half_ulps:
   * Even if the input is normal, its 0.5 ULP can be subnormal. */
  if (exp_hulp < 1) {
    half_ulp.ieee.exponent = 0;
    half_ulp.ieee.mantissa = 1 << (FLOAT_PMAN_SIZE - 1 + exp_hulp);
  }

  if (check_if_binary32_needs_rounding(b32x, precision)) {
    b32x.f32 += half_ulp.f32;
  }

  b32x.ieee.mantissa &= mask; // Zero out the least significant bits

  return b32x.f32;
}

inline static int check_if_binary64_needs_rounding(binary64 b64x,
                                                   int precision) {
  const uint64_t one = 1;

  // Create a mask to isolate the bits that will be rounded.
  const uint64_t trailing_bits_mask =
      (one << (DOUBLE_PMAN_SIZE - precision)) - 1;
  const uint64_t trailing_bits = b64x.ieee.mantissa & trailing_bits_mask;

  // Calculate the bit that will be rounded
  const uint64_t bit_to_round_offset = (DOUBLE_PMAN_SIZE - precision);
  const uint64_t bit_to_round = (b64x.ieee.mantissa >> bit_to_round_offset) & 1;

  // Calculate the halfway point for rounding.
  const uint64_t halfway_point = one << (DOUBLE_PMAN_SIZE - precision - 1);

  // If the trailing bits are greater than the halfway point, or exactly equal
  // to it and the bit-to-round is 1 (for tie-breaking), round up by adding
  // half_ulp to x.
  return (trailing_bits > halfway_point) ||
         ((trailing_bits == halfway_point) && (bit_to_round == 1));
}

/**
 * round the mantissa of 'x' on the precision specified by 'precision'
 * this function does not check that 'precision' is within the correct
 * range; the user is responsible for ensuring that 'precision'
 * satisfies this requirement
 *   x: the binary64 number to round
 *   precision: the new virtual precision; 0<=precision<=DOUBLE_PMAN_SIZE
 *   retval x rounded on the specified precision
 */
inline double round_binary64_normal(double x, int precision) {
  /* build 1/2 ulp and add it  before truncation for rounding to nearest
   * with ties to even */

  /* generate a mask to erase the last 52-VPRECLIB_PREC bits, in other
     words, there remain VPRECLIB_PREC bits in the mantissa */
  const uint64_t mask = 0xFFFFFFFFFFFFFFFF << (DOUBLE_PMAN_SIZE - precision);

  binary64 b64x = {.f64 = x};

  int exp_hulp = b64x.ieee.exponent - precision - 1;
  binary64 half_ulp = {
      .ieee = {.sign = x < 0, .exponent = exp_hulp, .mantissa = 0}};
  /* Handle denormal half_ulps:
   * Even if the input is normal, its 0.5 ULP can be subnormal. */
  if (exp_hulp < 1) {
    int64_t one = 1;
    binary64 half_ulp_denorm = {
        .ieee = {.sign = x < 0,
                 .exponent = 0,
                 .mantissa = one << (DOUBLE_PMAN_SIZE - 1 + exp_hulp)}};
    half_ulp = half_ulp_denorm;
  }

  if (check_if_binary64_needs_rounding(b64x, precision)) {
    b64x.f64 += half_ulp.f64;
  }

  b64x.ieee.mantissa &= mask; // Zero out the least significant bits

  return b64x.f64;
}

inline static int check_if_binary128_needs_rounding(binary128 b128_x,
                                                    int precision) {
  const __uint128_t one = 1;

  // Create a mask to isolate the bits that will be rounded.
  const __uint128_t trailing_bits_mask =
      (one << (QUAD_PMAN_SIZE - precision)) - 1;
  const __uint128_t trailing_bits =
      b128_x.ieee128.mantissa & trailing_bits_mask;

  // Calculate the bit that will be rounded
  const __uint128_t bit_to_round_offset = (QUAD_PMAN_SIZE - precision);
  const __uint128_t bit_to_round =
      (b128_x.ieee128.mantissa >> bit_to_round_offset) & 1;

  // Calculate the halfway point for rounding.
  const __uint128_t halfway_point = one << (QUAD_PMAN_SIZE - precision - 1);

  // If the trailing bits are greater than the halfway point, or exactly equal
  // to it and the bit-to-rouind is 1 (for tie-breaking), round up by adding
  // half_ulp to x.
  return (trailing_bits > halfway_point) ||
         ((trailing_bits == halfway_point) && (bit_to_round == 1));
}

/* This function handles rounding when an underflow occurs. */
/* This function assumes that the number is below the minimum representable
 * number. */
inline binary128 round_binary128_underflow(binary128 x, int emin,
                                           int precision) {
  binary128 b128x = x;
  binary128 half_smallest_subnormal = {
      .ieee128 = {.sign = b128x.ieee128.sign,
                  .exponent = QUAD_EXP_COMP + (emin - precision) - 1,
                  .mantissa = 0}};
  //  If x is greater than or equal to half of the smallest subnormal number,
  //  rounds to the smallest subnormal number with the same sign.
  //  Otherwise, rounds to zero while preserving the sign.
  //  checks if x is greater than or equal to half of the smallest subnormal
  if (b128x.ieee128.exponent >= half_smallest_subnormal.ieee128.exponent) {
    // then round to the smallest subnormal number with the same sign
    b128x.f128 += half_smallest_subnormal.f128;
    b128x.ieee128.mantissa = 0;
  } else {
    // otherwise, round to zero while preserving the sign
    b128x.f128 = 0;
  }
  return b128x;
}

inline static double round_binary_denormal(double x, int emin, int precision) {
  /* emin represents the lowest exponent in the normal range */

  binary128 b128_x = {.f128 = x};

  /* build 1/2 ulp and add it before truncation for rounding to nearest with
   * ties to even */
  const __uint128_t half_ulp_exp = QUAD_EXP_COMP + (emin - precision) - 1;
  const binary128 half_ulp = {
      .ieee128 = {.sign = x < 0, .exponent = half_ulp_exp, .mantissa = 0}};

  // Calculate the loss of precision due to the number being subnormal.
  const int32_t precision_loss =
      emin - (b128_x.ieee128.exponent - QUAD_EXP_COMP);

  /*
   * When precision_loss >= precision, the effective precision becomes <= 0, so
   * the generic "round then truncate" path is no longer valid:
   *   - check_if_binary128_needs_rounding() would be called with a non-positive
   *     precision (undefined/incorrect shifts).
   *   - the truncation mask would also require invalid shift amounts.
   */
  if (precision_loss >= precision) {
    return round_binary128_underflow(b128_x, emin, precision).f128;
  }

  if (check_if_binary128_needs_rounding(b128_x, precision - precision_loss)) {
    b128_x.f128 += half_ulp.f128;
  }

  /* truncate trailing bits */
  __uint128_t mask_denormal = 0;
  mask_denormal = ~mask_denormal
                  << (QUAD_PMAN_SIZE - precision + precision_loss);
  b128_x.ieee128.mantissa &= mask_denormal;

  // If x is less than or equal to the smallest subnormal number, round it to
  // the smallest subnormal number or zero, depending on whether it's greater
  // than or equal to half of the smallest subnormal number.
  // checks if x is less than or equal to the smallest subnormal number
  if (b128_x.ieee128.exponent <= (half_ulp.ieee128.exponent + 1)) {
    b128_x = round_binary128_underflow(b128_x, emin, precision);
  }

  return b128_x.f128;
}

inline int has_underflow_binary32(float x, int emin, int precision) {
  binary32 b32_x = {.f32 = x};
  return (b32_x.ieee.exponent - FLOAT_EXP_COMP) < (emin - precision);
}

/* This function handles rounding when an underflow occurs. */
/* This function assumes that the number is below the minimum representable
 * number. */
inline float round_binary32_underflow(float x, int emin, int precision) {
  binary32 b32_x = {.f32 = x};
  binary32 half_smallest_subnormal = {
      .ieee = {.sign = 0,
               .exponent = FLOAT_EXP_COMP + (emin - precision) - 1,
               .mantissa = 0}};

  //  If x is greater than or equal to half of the smallest subnormal number,
  //  rounds to the smallest subnormal number with the same sign.
  //  Otherwise, rounds to zero while preserving the sign.
  // checks if x is greater than or equal to half of the smallest subnormal
  if (b32_x.ieee.exponent >= half_smallest_subnormal.ieee.exponent) {
    // then round to the smallest subnormal number with the same sign
    b32_x.ieee.exponent = half_smallest_subnormal.ieee.exponent + 1;
    b32_x.ieee.mantissa = 0;
  } else {
    // otherwise, round to zero while preserving the sign
    b32_x.f32 = 0;
  }
  return b32_x.f32;
}

inline float handle_binary32_denormal(float x, int emin, int precision) {
  /* underflow */
  if (has_underflow_binary32(x, emin, precision)) {
    return round_binary32_underflow(x, emin, precision);
  }
  /* denormal */
  else if (precision <= FLOAT_PMAN_SIZE) {
    return round_binary_denormal(x, emin, precision);
  }
  /* no rounding needed, precision is greater than the mantissa size */
  else {
    return x;
  }
}

inline int has_underflow_binary64(double x, int emin, int precision) {
  binary64 b64_x = {.f64 = x};
  return (b64_x.ieee.exponent - DOUBLE_EXP_COMP) < (emin - precision);
}

/* This function handles rounding when an underflow occurs. */
/* This function assumes that the number is below the minimum representable
 * number. */
inline double round_binary64_underflow(double x, int emin, int precision) {
  binary64 b64_x = {.f64 = x};
  binary64 half_smallest_subnormal = {
      .ieee = {.sign = 0,
               .exponent = DOUBLE_EXP_COMP + (emin - precision) - 1,
               .mantissa = 0}};

  //  If x is greater than or equal to half of the smallest subnormal number,
  //  rounds to the smallest subnormal number with the same sign.
  //  Otherwise, rounds to zero while preserving the sign.
  // checks if x is greater than or equal to half of the smallest subnormal
  if (b64_x.ieee.exponent >= half_smallest_subnormal.ieee.exponent) {
    // then round to the smallest subnormal number with the same sign
    b64_x.ieee.exponent = half_smallest_subnormal.ieee.exponent + 1;
    b64_x.ieee.mantissa = 0;
  } else {
    // otherwise, round to zero while preserving the sign
    b64_x.f64 = 0;
  }
  return b64_x.f64;
}

inline double handle_binary64_denormal(double x, int emin, int precision) {
  /* underflow */
  if (has_underflow_binary64(x, emin, precision)) {
    return round_binary64_underflow(x, emin, precision);
  }
  /* denormal */
  else if (precision <= DOUBLE_PMAN_SIZE) {
    return round_binary_denormal(x, emin, precision);
  }
  /* no rounding needed, precision is greater than the mantissa size */
  else {
    return x;
  }
}
