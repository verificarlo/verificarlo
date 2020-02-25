/*****************************************************************************
 *                                                                           *
 *  This file is part of Verificarlo.                                        *
 *                                                                           *
 *  Copyright (c) 2015                                                       *
 *     Universite de Versailles St-Quentin-en-Yvelines                       *
 *     CMLA, Ecole Normale Superieure de Cachan                              *
 *  Copyright (c) 2018-2020                                                  *
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

#ifndef __FLOAT_UTILS_H__
#define __FLOAT_UTILS_H__

#include <math.h>

#include "float_struct.h"
#include "logger.h"

/* perform_bin_op: applies the binary operator (op) to (a) and (b) */
/* and stores the result in (res) */
#define PERFORM_BIN_OP(BACKEND, OP, RES, A, B)                                 \
  switch (OP) {                                                                \
  case BACKEND##_add:                                                          \
    RES = (A) + (B);                                                           \
    break;                                                                     \
  case BACKEND##_mul:                                                          \
    RES = (A) * (B);                                                           \
    break;                                                                     \
  case BACKEND##_sub:                                                          \
    RES = (A) - (B);                                                           \
    break;                                                                     \
  case BACKEND##_div:                                                          \
    RES = (A) / (B);                                                           \
    break;                                                                     \
  default:                                                                     \
    logger_error("invalid operator %c", OP);                                   \
  };

/* Generic getters for float constants */
#define GET_EXP_MAX(X)                                                         \
  _Generic((X), float : FLOAT_EXP_MAX, double : DOUBLE_EXP_MAX)
#define GET_EXP_MIN(X)                                                         \
  _Generic((X), float : FLOAT_EXP_MIN, double : DOUBLE_EXP_MIN)
#define GET_SIGN_SIZE(X)                                                       \
  _Generic((X), float : FLOAT_SIGN_SIZE, double : DOUBLE_SIGN_SIZE)
#define GET_EXP_SIZE(X)                                                        \
  _Generic((X), float : FLOAT_EXP_SIZE, double : DOUBLE_EXP_SIZE)
#define GET_PMAN_SIZE(X)                                                       \
  _Generic((X), float : FLOAT_PMAN_SIZE, double : DOUBLE_PMAN_SIZE)
#define GET_EXP_COMP(X)                                                        \
  _Generic((X), float : FLOAT_EXP_COMP, double : DOUBLE_EXP_COMP)
#define GET_PREC(X) _Generic(X, float : FLOAT_PREC, double : DOUBLE_PREC)
#define GET_MASK_ONE(X)                                                        \
  _Generic(X, float : FLOAT_MASK_ONE, double : DOUBLE_MASK_ONE)

/* Unified fpclassify function for binary32, binary64 and binary128 */
#define FPCLASSIFY(X)                                                          \
  _Generic(X, float                                                            \
           : __builtin_fpclassify(FP_NAN, FP_INFINITE, FP_NORMAL,              \
                                  FP_SUBNORMAL, FP_ZERO, X),                   \
             double                                                            \
           : __builtin_fpclassify(FP_NAN, FP_INFINITE, FP_NORMAL,              \
                                  FP_SUBNORMAL, FP_ZERO, X),                   \
             __float128                                                        \
           : __builtin_fpclassify(QUADFP_NAN, QUADFP_INFINITE, QUADFP_NORMAL,  \
                                  QUADFP_SUBNORMAL, QUADFP_ZERO, X))

/* Denormals-Are-Zero */
/* Returns zero if X is a subnormal value */
#define DAZ(X) (FPCLASSIFY(X) == FP_SUBNORMAL) ? 0 : X
/* Flush-To-Zero */
/* Returns zero if X is a subnormal value */
#define FTZ(X) (FPCLASSIFY(X) == FP_SUBNORMAL) ? 0 : X

/* Return true if the value x is representable on the precision
 * virtual_precision  */
bool _is_representable_binary32(const float x, const int virtual_precision);
bool _is_representable_binary64(const double x, const int virtual_precision);
bool _is_representable_binary128(const __float128 x,
                                 const int virtual_precision);

/* Generic call for _is_representable_TYPEOF(X) */
#define _IS_REPRESENTABLE(X, VT)                                               \
  _Generic(X, float                                                            \
           : _is_representable_binary32, double                                \
           : _is_representable_binary64, __float128                            \
           : _is_representable_binary128)(X, VT)

/* Return the unbiased exponent of x */
int32_t _get_exponent_binary32(const float x);
int32_t _get_exponent_binary64(const double x);
int32_t _get_exponent_binary128(const __float128 x);

/* Generic call for get_exponent_TYPEOF(X) */
#define GET_EXP_FLT(X)                                                         \
  _Generic(X, float                                                            \
           : _get_exponent_binary32, double                                    \
           : _get_exponent_binary64, __float128                                \
           : _get_exponent_binary128)(X)

/* Returns 2^exp */
/* Fast function that implies no overflow neither underflow */
float _fast_pow2_binary32(const int exp);
double _fast_pow2_binary64(const int exp);
__float128 _fast_pow2_binary128(const int exp);

#endif /* __FLOAT_UTILS_H__ */
