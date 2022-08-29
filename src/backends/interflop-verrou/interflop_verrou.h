
/*--------------------------------------------------------------------*/
/*--- Verrou: a FPU instrumentation tool.                          ---*/
/*--- Interface for floatin-point operations overloading.          ---*/
/*---                                           interflop_verrou.h ---*/
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


#ifndef __INTERFLOP_VERROU_H
#define __INTERFLOP_VERROU_H

//#define DEBUG_PRINT_OP

#ifdef __cplusplus
extern "C" {
#endif
#define IFV_FCTNAME(FCT) interflop_verrou_##FCT

  //#include "../interflop_backend_interface.h"
#include "../../common/interflop.h"

  
  enum vr_RoundingMode {
    VR_NEAREST,
    VR_UPWARD,
    VR_DOWNWARD,
    VR_ZERO,
    VR_RANDOM, // Must be immediately after standard rounding modes
    VR_AVERAGE,
    VR_FARTHEST,
    VR_FLOAT,
    VR_NATIVE,
    VR_FTZ
  };



  void IFV_FCTNAME(configure)(enum vr_RoundingMode mode,void* context);
  void IFV_FCTNAME(finalyze)(void* context);

  const char* IFV_FCTNAME(get_backend_name)(void);
  const char* IFV_FCTNAME(get_backend_version)(void);


  const char* verrou_rounding_mode_name (enum vr_RoundingMode mode);

  void verrou_begin_instr(void);
  void verrou_end_instr(void);

  void verrou_set_seed (unsigned int seed);
  void verrou_set_random_seed (void);

  extern void (*vr_panicHandler)(const char*);
  void verrou_set_panic_handler(void (*)(const char*));
  extern void (*vr_nanHandler)(void);
  void verrou_set_nan_handler(void (*nanHandler)(void));

  extern void (*vr_debug_print_op)(int,const char*, const double* args, const double* res);
  void verrou_set_debug_print_op(void (*)(int nbArg, const char* name, const double* args, const double* res));

  struct interflop_backend_interface_t IFV_FCTNAME(init)(void ** context);

  void IFV_FCTNAME(add_double) (double a, double b, double* res, void* context);    
  void IFV_FCTNAME(add_float)  (float a,  float b,  float*  res, void* context);
  void IFV_FCTNAME(sub_double) (double a, double b, double* res, void* context);
  void IFV_FCTNAME(sub_float)  (float a,  float b,  float*  res, void* context);
  void IFV_FCTNAME(mul_double) (double a, double b, double* res, void* context);
  void IFV_FCTNAME(mul_float)  (float a,  float b,  float*  res, void* context);
  void IFV_FCTNAME(div_double) (double a, double b, double* res, void* context);
  void IFV_FCTNAME(div_float)  (float a,  float b,  float*  res, void* context);

  void IFV_FCTNAME(cast_double_to_float) (double a, float* b, void* context);

  void IFV_FCTNAME(madd_double)(double a, double b, double c, double* res, void* context);
  void IFV_FCTNAME(madd_float) (float a,  float b,  float c,  float*  res, void* context);

  
#ifdef __cplusplus
}
#endif

#endif /* ndef __INTERFLOP_VERROU_H */
