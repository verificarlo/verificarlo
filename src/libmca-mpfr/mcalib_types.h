// The Monte Carlo Arihmetic Library - A tool for automated rounding error
// analysis of floating point software. Copyright (C) 2014 The Computer
// Engineering Laboratory, The University of Sydney. Maintained by Michael
// Frechtling:
// 
// 	michael.frechtling@sydney.edu.au
// 
// This file is part of the Monte Carlo Arithmetic Library, (MCALIB). MCALIB is
// free software: you can redistribute it and/or modify it under the terms of
// the GNU General Public License as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
// details.
// 
// You should have received a copy of the GNU General Public License along with
// this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef MCALIB_TYPES
#define MCALIB_TYPES
#include "mpfr.h"
#include "stdio.h"
#include "math.h"
#include "stdlib.h"
#include <sys/time.h>

#define MCALIB_IEEE 	0
#define MCALIB_MCA 	1
#define MCALIB_PB	2
#define MCALIB_RR	3

#define MP_ADD &mpfr_add
#define MP_SUB &mpfr_sub
#define MP_MUL &mpfr_mul
#define MP_DIV &mpfr_div
#define MP_NEG &mpfr_neg

typedef int (*mpfr_bin)(mpfr_t, mpfr_t, mpfr_t, mpfr_rnd_t);
typedef int (*mpfr_unr)(mpfr_t, mpfr_t, mpfr_rnd_t);

extern float _mca_sbin(float a, float b, mpfr_bin mpfr_op);
extern float _mca_sunr(float a, mpfr_unr mpfr_op);
extern int _mca_scmp(float a, float b);
extern double _mca_dbin(double a, double b, mpfr_bin mpfr_op);
extern double _mca_dunr(double a, mpfr_unr mpfr_op);
extern int _mca_dcmp(double a, double b);
#endif
