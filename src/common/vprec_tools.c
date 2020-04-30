/*****************************************************************************
 *                                                                           *
 *  This file is part of Verificarlo.                                        *
 *                                                                           *
 *  Copyright (c) 2015                                                       *
 *     Universite de Versailles St-Quentin-en-Yvelines                       *
 *     CMLA, Ecole Normale Superieure de Cachan                              *
 *  Copyright (c) 2019                                                       *
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

inline float round_binary32_denormal(float x, int emin, int xexp,
                                     int precision) {

  /* build 1/2 ulp and add it  before truncation for faithfull rounding */

  /* position to the end of the target prec-1 */
  const uint32_t target_position = FLOAT_PMAN_SIZE - precision - 1;

  /*precision loss due to denormalizqation*/
  const uint32_t precision_loss = emin - xexp;

  /* truncate the trailing bits */
  const uint32_t mask_denormal =
      0xFFFFFFFF << (FLOAT_PMAN_SIZE - precision + precision_loss);

  binary32 b32_x = {.f32 = x};
  b32_x.ieee.mantissa = 0;
  binary32 half_ulp = {.f32 = x};
  half_ulp.ieee.mantissa = 1 << (target_position + precision_loss);

  b32_x.f32 = x + (half_ulp.f32 - b32_x.f32);
  b32_x.u32 &= mask_denormal;

  return b32_x.f32;
}

inline float round_binary32_normal(float x, int precision) {

  /* build 1/2 ulp and add it  before truncation for faithfull rounding */

  /* generate a mask to erase the last 23-VPRECLIB_PREC bits, in other word,
     it remains VPRECLIB_PREC bit in the mantissa */
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

inline double round_binary64_denormal(double x, int emin, int xexp,
                                      int precision) {

  /* build 1/2 ulp and add it before truncation for faithfull rounding */

  /* position to the end of the target prec-1 */
  const uint64_t target_position = DOUBLE_PMAN_SIZE - precision - 1;

  /*precision loss due to denormalization*/
  const uint64_t precision_loss = emin - xexp;

  /* truncate the trailing bits */
  const uint64_t mask_denormal =
      0xFFFFFFFFFFFFFFFF << (DOUBLE_PMAN_SIZE - precision + precision_loss);

  binary64 b64x = {.f64 = x};
  b64x.ieee.mantissa = 0;
  binary64 half_ulp = {.f64 = x};
  half_ulp.ieee.mantissa = 1ULL << (target_position + precision_loss);

  b64x.f64 = x + (half_ulp.f64 - b64x.f64);
  b64x.u64 &= mask_denormal;

  return b64x.f64;
}

inline double round_binary64_normal(double x, int precision) {

  /* build 1/2 ulp and add it  before truncation for faithfull rounding */

  /* generate a mask to erase the last 52-VPRECLIB_PREC bits, in other word,
     it remains VPRECLIB_PREC bit in the mantissa */
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

inline float handle_binary32_denormal(float x, int emin, int xexp,
                                      int precision) {
  /* underflow */
  if (xexp < (emin - precision)) {
    /* multiply by 0 a to keep the sign */
    return x * 0;
  }
  /* denormal */
  else if (precision <= FLOAT_PMAN_SIZE) {
    return round_binary32_denormal(x, emin, xexp, precision);
  }
}

inline double handle_binary64_denormal(double x, int emin, int xexp,
                                       int precision) {
  /* underflow */
  if (xexp < (emin - precision)) {
    /* multiply by a 0 to keep the sign */
    return x * 0;
  }
  /* denormal */
  else if (precision <= DOUBLE_PMAN_SIZE) {
    return round_binary64_denormal(x, emin, xexp, precision);
  }
}
