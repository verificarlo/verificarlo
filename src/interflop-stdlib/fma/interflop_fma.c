/*****************************************************************************\
 *                                                                           *\
 *  This file is part of the Verificarlo project,                            *\
 *  under the Apache License v2.0 with LLVM Exceptions.                      *\
 *  SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception.                 *\
 *  See https://llvm.org/LICENSE.txt for license information.                *\
 *                                                                           *\
 *  Copyright (c) 2024                                                       *\
 *     Verificarlo Contributors                                              *\
 *                                                                           *\
 ****************************************************************************/

#include "interflop_fma.h"
#include "pfp128.h"

float interflop_fma_binary32(float a, float b, float c) {
  return __builtin_fmaf(a, b, c);
}

double interflop_fma_binary64(double a, double b, double c) {
  return __builtin_fma(a, b, c);
}

_Float128 interflop_fma_binary128(_Float128 a, _Float128 b, _Float128 c) {
#ifdef HAS_QUADMATH
  return fmaq(a, b, c);
#else
  return fmaFP128(a, b, c);
#endif
}