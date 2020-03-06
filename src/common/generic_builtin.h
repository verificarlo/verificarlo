/*****************************************************************************
 *                                                                           *
 *  This file is part of Verificarlo.                                        *
 *                                                                           *
 *  Copyright (c) 2015                                                       *
 *     Universite de Versailles St-Quentin-en-Yvelines                       *
 *     CMLA, Ecole Normale Superieure de Cachan                              *
 *  Copyright (c) 2018-2020                                                  *
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

#ifndef __GENERIC_BUILTIN_H__
#define __GENERIC_BUILTIN_H__

#include <stdint.h>

#include "float_const.h"

/* This file group generic builtin functions */

/* Returns the first least 1-bit + 1 = position of the first trailing 0-bit */
#define FFS(X)                                                                 \
  _Generic(X, uint32_t : __builtin_ffs(X), uint64_t : __builtin_ffsl(X))

/* Returns the number of leading 0-bits in x,
   starting at the most significant bit position.
   If x is 0, the result is undefined. */
#define CLZ(X)                                                                 \
  _Generic(X, uint32_t : __builtin_clz(X), uint64_t : __builtin_clzl(X))

/* Variant of CLZ that takes two arguments */
/* X is used for selecting the type */
/* Y is the actual argument */
#define CLZ2(X, Y)                                                             \
  _Generic(X, uint32_t : __builtin_clz, uint64_t : __builtin_clzl)(Y)

/* Returns the number of trailing 0-bits in x,
   starting at the most significant bit position.
   If x is 0, the result is undefined. */
#define CTZ(X)                                                                 \
  _Generic(X, uint32_t : __builtin_ctz(X), uint64_t : __builtin_ctzl(X))

#endif /* __GENERIC_BUILTIN_H__ */
