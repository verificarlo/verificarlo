/*--------------------------------------------------------------------*/
/*--- Verrou: a FPU instrumentation tool.                          ---*/
/*--- Interface for random number generation.                      ---*/
/*---                                                    vr_rand.h ---*/
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

#ifndef __VR_RAND_H
#define __VR_RAND_H

#include <cstdint>

#include "interflop/prng/tinymt64.h"
#include "interflop/prng/xoshiro.hxx"

typedef struct Vr_Rand_ Vr_Rand;
struct Vr_Rand_ {
  uint64_t current_;
  tinymt64_t gen_;
#ifdef USE_XOSHIRO
  xoshiro128_state_t rng128_;
  xoshiro256_state_t rng256_;
#endif
  uint64_t seed_;
  double p;
  uint32_t count_;
};

/* Disable thread safety for RNG required for Valgrind */
#ifdef RNG_THREAD_SAFE
#define TLS __thread
#else
#define TLS
#endif

extern TLS Vr_Rand vr_rand;

#endif
