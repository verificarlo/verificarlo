
/*--------------------------------------------------------------------*/
/*--- Verrou: a FPU instrumentation tool.                          ---*/
/*--- Utilities for easier manipulation of floating-point values.  ---*/
/*---                                                vr_isNan.hxx ---*/
/*--------------------------------------------------------------------*/

/*
   This file is part of Verrou, a FPU instrumentation tool.

   Copyright (C) 2014-2021 EDF
     F. Févotte     <francois.fevotte@edf.fr>
     B. Lathuilière <bruno.lathuiliere@edf.fr>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public License as
   published by the Free Software Foundation; either version 2.1 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307, USA.

   The GNU Lesser General Public License is contained in the file COPYING.
*/

#pragma once

#include <cfloat>
#include <stdint.h>


#include "interflop_verrou.h"


template <class REALTYPE>
inline bool isNan (const REALTYPE & x) {
  vr_panicHandler("isNan called on an unknown type");
  return false;
}

template <>
inline bool isNan<double> (const double & x) {
  static const uint64_t maskSpecial = 0x7ff0000000000000;
  static const uint64_t maskInf     = 0x000fffffffffffff;
  const uint64_t* X = reinterpret_cast<const uint64_t*>(&x);
  if ((*X & maskSpecial) == maskSpecial) {
    if ((*X & maskInf) != 0) {
      return true;
    }
  }
  return false;
}

template <>
inline bool isNan<float> (const float & x) {
  static const uint32_t maskSpecial = 0x7f800000;
  static const uint32_t maskInf     = 0x007fffff;
  const uint32_t* X = reinterpret_cast<const uint32_t*>(&x);
  if ((*X & maskSpecial) == maskSpecial) {
    if ((*X & maskInf) != 0) {
      return true;
    }
  }
  return false;
}



template<class REALTYPE>
inline bool isNanInf (const REALTYPE & x) {
  vr_panicHandler("isNanInf called on an unknown type");
  return false;
}

template <>
inline bool isNanInf<double> (const double & x) {
  static const uint64_t mask = 0x7ff0000000000000;
  const uint64_t* X = reinterpret_cast<const uint64_t*>(&x);
  return (*X & mask) == mask;
}

template <>
inline bool isNanInf<float> (const float & x) {
  static const uint32_t mask = 0x7f800000;
  const uint32_t* X = reinterpret_cast<const uint32_t*>(&x);	
  return (*X & mask) == mask;  
}
