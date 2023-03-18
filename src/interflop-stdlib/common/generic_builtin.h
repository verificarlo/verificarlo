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
 *  Copyright (c) 2019-2021                                                  *\
 *     Verificarlo Contributors                                              *\
 *                                                                           *\
 ****************************************************************************/
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
