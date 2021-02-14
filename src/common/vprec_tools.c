/*****************************************************************************
 *                                                                           *
 *  This file is part of Verificarlo.                                        *
 *                                                                           *
 *  Copyright (c) 2015                                                       *
 *     Universite de Versailles St-Quentin-en-Yvelines                       *
 *     CMLA, Ecole Normale Superieure de Cachan                              *
 *  Copyright (c) 2019-2020						     *
 *     Verificarlo contributors                                              *
 *     Universite de Versailles St-Quentin-en-Yvelines                       *
 *                                                                           *
 *  Verificarlo is free software: you can redistribute it and/or modify      *
 *  it under the terms of the GNU General Public License as published by     *
 *  the Free Software Foundation, either version 3 of the License, or        *
 *  (at your option) any later version.                                      *
 *                                                                           *
 *  Verificarlo is distributed in the hope that it will be useful,           *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *  GNU General Public License for more details.                             *
 *                                                                           *
 *  You should have received a copy of the GNU General Public License        *
 *  along with Verificarlo.  If not, see <http://www.gnu.org/licenses/>.     *
 *                                                                           *
 *****************************************************************************/

#include "vprec_tools.h"
#include "float_const.h"
#include "float_struct.h"

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
  /* build 1/2 ulp and add it  before truncation for faithfull rounding */

  /* generate a mask to erase the last 23-VPRECLIB_PREC bits, in other words,
     there remain VPRECLIB_PREC bits in the mantissa */
  const uint32_t mask = 0xFFFFFFFF << (FLOAT_PMAN_SIZE - precision);

  /* position to the end of the target prec-1 */
  const uint32_t target_position = FLOAT_PMAN_SIZE - precision - 1;

  binary32 b32x = {.f32 = x};
  b32x.ieee.mantissa = 0;
  binary32 half_ulp = {.f32 = x};
  half_ulp.ieee.mantissa = (1 << target_position);

  b32x.f32 = x + (half_ulp.f32 - b32x.f32);
  b32x.u32 &= mask;

  return b32x.f32;
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
  /* build 1/2 ulp and add it  before truncation for faithfull rounding */

  /* generate a mask to erase the last 52-VPRECLIB_PREC bits, in other words,
     there remain VPRECLIB_PREC bits in the mantissa */
  const uint64_t mask = 0xFFFFFFFFFFFFFFFF << (DOUBLE_PMAN_SIZE - precision);

  /* position to the end of the target prec-1 */
  const uint64_t target_position = DOUBLE_PMAN_SIZE - precision - 1;

  binary64 b64x = {.f64 = x};
  b64x.ieee.mantissa = 0;
  binary64 half_ulp = {.f64 = x};
  half_ulp.ieee.mantissa = 1ULL << target_position;

  b64x.f64 = x + (half_ulp.f64 - b64x.f64);
  b64x.u64 &= mask;

  return b64x.f64;
}

inline static double round_binary_denormal(double x, int emin, int precision) {
  /* emin represents the lowest exponent in the normal range */

  /* build 1/2 ulp and add it before truncation for faithful rounding */
  binary128 half_ulp;
  half_ulp.ieee128.exponent = QUAD_EXP_COMP + (emin - precision) - 1;
  half_ulp.ieee128.sign = x < 0;
  half_ulp.ieee128.mantissa = 0;
  binary128 b128_x = {.f128 = x + half_ulp.f128};

  /* truncate trailing bits */
  const int32_t precision_loss =
      emin - (b128_x.ieee128.exponent - QUAD_EXP_COMP);
  __uint128_t mask_denormal = 0;
  mask_denormal = ~mask_denormal
                  << (QUAD_PMAN_SIZE - precision + precision_loss);
  b128_x.ieee128.mantissa &= mask_denormal;

  return b128_x.f128;
}

inline float handle_binary32_denormal(float x, int emin, int precision) {
  binary32 b32_x = {.f32 = x};
  /* underflow */
  if ((b32_x.ieee.exponent - FLOAT_EXP_COMP) < (emin - precision)) {
    /* multiply by 0 to keep the sign */
    return x * 0;
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

inline double handle_binary64_denormal(double x, int emin, int precision) {
  binary64 b64_x = {.f64 = x};
  /* underflow */
  if ((b64_x.ieee.exponent - DOUBLE_EXP_COMP) < (emin - precision)) {
    /* multiply by 0 to keep the sign */
    return x * 0;
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
