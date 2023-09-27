/*****************************************************************************\
 *                                                                           *\
 *  This file is part of the Verificarlo project,                            *\
 *  under the Apache License v2.0 with LLVM Exceptions.                      *\
 *  SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception.                 *\
 *  See https://llvm.org/LICENSE.txt for license information.                *\
 *                                                                           *\
 *  Copyright (c) 2022                                                  *\
 *     Verificarlo Contributors                                              *\
 *                                                                           *\
 ****************************************************************************/

#ifndef __XOROSHIRO128_H__
#define __XOROSHIRO128_H__

#include <stdint.h>

typedef uint64_t xoroshiro_state[2];

uint64_t next(xoroshiro_state state);
double next_double(xoroshiro_state state);

#endif /* __XOROSHIRO128_H__ */
