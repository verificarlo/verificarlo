/*****************************************************************************
 *                                                                           *
 *  This file is part of Verificarlo.                                        *
 *                                                                           *
 *  Copyright (c) 2015                                                       *
 *     Universite de Versailles St-Quentin-en-Yvelines                       *
 *     CMLA, Ecole Normale Superieure de Cachan                              *
 *  Copyright (c) 2019-2020                                                  *
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

#ifndef __VPREC_TOOLS_H__
#define __VPREC_TOOLS_H__

#include "float_struct.h"
#include "float_type.h"

/******************** VPREC ARITHMETIC FUNCTIONS ********************
 * The following set of functions perform the VPREC operation. Operands
 * are first correctly rounded to the target precison format if inbound
 * is set, the operation is then perform using IEEE hw and
 * correct rounding to the target precision format is done if outbound
 * is set.
 *******************************************************************/

float round_binary32_normal(float x, int precision);
float handle_binary32_denormal(float x, int emin, int precision);

void round_binary32_normal_2x(float2 *x, int2 precision);
void round_binary32_normal_4x(float4 *x, int4 precision);
void round_binary32_normal_8x(float8 *x, int8 precision);
void round_binary32_normal_16x(float16 *x, int16 precision);

double round_binary64_normal(double x, int precision);
double handle_binary64_denormal(double x, int emin, int precision);

void round_binary64_normal_2x(double2 *x, int64_2x precision);
void round_binary64_normal_4x(double4 *x, int64_4x precision);
void round_binary64_normal_8x(double8 *x, int64_8x precision);
void round_binary64_normal_16x(double16 *x, int64_16x precision);

#endif /* __VPREC_TOOLS_H__ */
