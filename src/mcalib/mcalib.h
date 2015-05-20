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

#ifndef MCALIB
#define MCALIB

#define MP_ADD &mpfr_add
#define MP_SUB &mpfr_sub
#define MP_MUL &mpfr_mul
#define MP_DIV &mpfr_div
#define MP_NEG &mpfr_neg

#define MCALIB_IEEE	0
#define MCALIB_MCA 	1
#define MCALIB_PB 	2
#define MCALIB_RR	3

extern int MCALIB_OP_TYPE;
extern int MCALIB_T;

extern void _mca_seed(void);

extern int _floateq(float a, float b);
extern int _floatne(float a, float b);
extern int _floatlt(float a, float b);
extern int _floatgt(float a, float b);
extern int _floatle(float a, float b);
extern int _floatge(float a, float b);

extern int _doubleeq(double a, double b);
extern int _doublene(double a, double b);
extern int _doublelt(double a, double b);
extern int _doublegt(double a, double b);
extern int _doublele(double a, double b);
extern int _doublege(double a, double b);

extern int _longeq(long double a, long double b);
extern int _longne(long double a, long double b);
extern int _longlt(long double a, long double b);
extern int _longgt(long double a, long double b);
extern int _longle(long double a, long double b);
extern int _longge(long double a, long double b);

extern float _floatadd(float a, float b);
extern float _floatsub(float a, float b);
extern float _floatmul(float a, float b);
extern float _floatdiv(float a, float b);
extern float _floatneg(float a);

extern double _doubleadd(double a, double b);
extern double _doublesub(double a, double b);
extern double _doublemul(double a, double b);
extern double _doublediv(double a, double b);
extern double _doubleneg(double a);

extern long double _longadd(long double a, long double b);
extern long double _longsub(long double a, long double b);
extern long double _longmul(long double a, long double b);
extern long double _longdiv(long double a, long double b);
extern long double _longneg(long double a);
#endif 
