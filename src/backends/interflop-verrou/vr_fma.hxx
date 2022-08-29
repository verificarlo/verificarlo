
/*--------------------------------------------------------------------*/
/*--- Verrou: a FPU instrumentation tool.                          ---*/
/*--- This file contains low-level code calling FMA instructions.  ---*/
/*---                                                   vr_fma.hxx ---*/
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

#ifdef    USE_VERROU_FMA
#include  <immintrin.h>
//#include  <fmaintrin.h>

template<class REALTYPE>
inline REALTYPE vr_fma(const REALTYPE&, const REALTYPE&, const REALTYPE&){
  return 0./ 0.; //nan to be sur not used
}

template<>
inline double vr_fma<double>(const double& a, const double& b, const double& c){
  double d;
  __m128d ai, bi,ci,di;
  ai = _mm_load_sd(&a);
  bi = _mm_load_sd(&b);
  ci = _mm_load_sd(&c);
  di=_mm_fmadd_sd(ai,bi,ci);
  d=_mm_cvtsd_f64(di);
  return d;
}


template<>
inline float vr_fma<float>(const float& a, const float& b, const float& c){
  float d;
  __m128 ai, bi,ci,di;
  ai = _mm_load_ss(&a);
  bi = _mm_load_ss(&b);
  ci = _mm_load_ss(&c);
  di=_mm_fmadd_ss(ai,bi,ci);
  d=_mm_cvtss_f32(di);
  return d;
}
#endif //USE_VERROU_FMA
