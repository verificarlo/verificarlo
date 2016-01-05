/********************************************************************************
 *                                                                              *
 *  This file is part of Verificarlo.                                           *
 *                                                                              *
 *  Copyright (c) 2015                                                          *
 *     Universite de Versailles St-Quentin-en-Yvelines                          *
 *     CMLA, Ecole Normale Superieure de Cachan                                 *
 *                                                                              *
 *  Verificarlo is free software: you can redistribute it and/or modify         *
 *  it under the terms of the GNU General Public License as published by        *
 *  the Free Software Foundation, either version 3 of the License, or           *
 *  (at your option) any later version.                                         *
 *                                                                              *
 *  Verificarlo is distributed in the hope that it will be useful,              *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               *
 *  GNU General Public License for more details.                                *
 *                                                                              *
 *  You should have received a copy of the GNU General Public License           *
 *  along with Verificarlo.  If not, see <http://www.gnu.org/licenses/>.        *
 *                                                                              *
 ********************************************************************************/


// Changelog:
//
// 2015-05-20 replace random number generator with TinyMT64. This
// provides a reentrant, independent generator of better quality than
// the one provided in libc.
//
// 2015-10-11 New version based on quad floating point type to replace MPFR until
// required MCA precision is lower than quad mantissa divided by 2, i.e. 56 bits
//
// 2015-16-11 New version using double precision for single precision operation

#include <math.h>
#include <mpfr.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdint.h>

#include "../common/quadmath-imp.h"
#include "libmca-quad.h"
#include "../vfcwrapper/vfcwrapper.h"
#include "../common/tinymt64.h"
#include "../common/mca_const.h"

int 	MCALIB_OP_TYPE 		= MCAMODE_IEEE;
int 	MCALIB_T		    = 53;

//possible op values
#define MCA_ADD 1
#define MCA_SUB 2
#define MCA_MUL 3
#define MCA_DIV 4


static float _mca_sbin(float a, float b, int qop);

static double _mca_dbin(double a, double b, int qop);

/******************** MCA CONTROL FUNCTIONS *******************
* The following functions are used to set virtual precision and
* MCA mode of operation.
***************************************************************/

static int _set_mca_mode(int mode){
	if (mode < 0 || mode > 3)
		return -1;

	MCALIB_OP_TYPE = mode;
	return 0;
}

static int _set_mca_precision(int precision){
	MCALIB_T = precision;
	return 0;
}

/******************** MCA RANDOM FUNCTIONS ********************
* The following functions are used to calculate the random
* perturbations used for MCA and apply these to MPFR format
* operands
***************************************************************/

/* random generator internal state */
tinymt64_t random_state;

static double _mca_rand(void) {
	/* Returns a random double in the (0,1) open interval */
	return tinymt64_generate_doubleOO(&random_state);
}

static inline double pow2d(int exp) {
  double res=0;
  uint64_t *x=malloc(sizeof(uint64_t));

  //specials
  if (exp == 0) return 1;

  if (exp > 1023) { /*exceed max exponent*/
	*x= DOUBLE_PLUS_INF;
  	res=*((double*)x);
	return res;
  }
  if (exp < -1022) { /*subnormal*/
        *x=((uint64_t) DOUBLE_PMAN_MSB ) >> -(exp+DOUBLE_EXP_MAX);
	res=*((double*)x);
        return res;
}

  //normal case
  //complement the exponent, sift it at the right place in the MSW
  *x=( ((uint64_t) exp) + DOUBLE_EXP_COMP) << DOUBLE_PMAN_SIZE;
  res=*((double*)x);
  return res;
}

static inline uint32_t rexpq (__float128 x)
{
  //no need to check special value in our cases since pow2q will deal with it
  //do not reuse it outside this code!
  uint64_t hx,ix;
  uint32_t exp=0;
  GET_FLT128_MSW64(hx,x);
  //remove sign bit, mantissa will be erased by the next shift
  ix = hx&QUAD_HX_ERASE_SIGN;
  //shift exponent to have LSB on position 0 and complement
  exp += (ix>>QUAD_HX_PMAN_SIZE)-QUAD_EXP_COMP;
  return exp;
}

static inline uint32_t rexpd (double x)
{
  //no need to check special value in our cases since pow2d will deal with it
  //do not reuse it outside this code!
  uint64_t hex,ix;
  uint32_t exp=0;
  //change type to bit field
  hex=*((uint64_t*) &x);
  //remove sign bit, mantissa will be erased by the next shift
  ix = hex&DOUBLE_ERASE_SIGN;
  //shift exponent to have LSB on position 0 and complement
  exp += (ix>>DOUBLE_PMAN_SIZE)-DOUBLE_EXP_COMP;
  return exp;
}

__float128 qnoise(int exp){
  double d_rand = (_mca_rand() - 0.5);
  uint64_t u_rand= *((uint64_t*) &d_rand);
  __float128 noise;
  uint64_t hx, lx;

  //specials
  if (exp == 0) return 1;

  if (exp > QUAD_EXP_MAX) { /*exceed max exponent*/
	SET_FLT128_WORDS64(noise, QINF_hx, QINF_lx);
	return noise;
  }
  if (exp < -QUAD_EXP_MIN) { /*subnormal*/
	//WARNING missing the random noise bits, only set the first one
	if (exp+QUAD_EXP_MAX<-QUAD_HX_PMAN_SIZE)
        	SET_FLT128_WORDS64(noise, ((uint64_t) 0 ) , WORD64_MSB  >> -(exp+QUAD_EXP_MAX+QUAD_HX_PMAN_SIZE));
	else
		SET_FLT128_WORDS64(noise, ((uint64_t) QUAD_HX_PMAN_MSB ) >> -(exp+QUAD_EXP_MAX) , ((uint64_t) 0 ));
	return noise;
  }

  //normal case
  //complement the exponent, shift it at the right place in the MSW
  hx=( ((uint64_t) exp+rexpd(d_rand)) + QUAD_EXP_COMP) << QUAD_HX_PMAN_SIZE;
  //set sign = sign of d_rand
  hx+=u_rand&DOUBLE_GET_SIGN;
  //extract u_rand (pseudo) mantissa and put the first 48 bits in hx...
  uint64_t p_mantissa=u_rand&DOUBLE_GET_PMAN;
  hx+=(p_mantissa)>>(DOUBLE_PMAN_SIZE-QUAD_HX_PMAN_SIZE);//4=52 (double pmantissa) - 48
  //...and the last 4 in lx at msb
  //uint64_t 
  lx=(p_mantissa)<<(SIGN_SIZE+DOUBLE_EXP_SIZE+QUAD_HX_PMAN_SIZE);//60=1(s)+11(exp double)+48(hx)
  SET_FLT128_WORDS64(noise, hx, lx);
  return noise;
}

static int _mca_inexactq(__float128 *qa) {

	if (MCALIB_OP_TYPE == MCAMODE_IEEE) {
		return 0;
	}

	//if (qa == 0) {
	//	return 0;
	//}

	int32_t e_a=0;
	e_a=rexpq(*qa);
	int32_t e_n = e_a - MCALIB_T;
	__float128 noise = qnoise(e_n);
	*qa=noise+*qa;
}

static int _mca_inexactd(double *da) {

	if (MCALIB_OP_TYPE == MCAMODE_IEEE) {
		return 0;
	}
	int32_t e_a=0;
	e_a=rexpd(*da);
	int32_t e_n = e_a - MCALIB_T;
	double d_rand = (_mca_rand() - 0.5);
	*da = *da + pow2d(e_n)*d_rand;
}

static void _mca_seed(void) {
	const int key_length = 3;
	uint64_t init_key[key_length];
	struct timeval t1;
	gettimeofday(&t1, NULL);

	/* Hopefully the following seed is good enough for Montercarlo */
	init_key[0] = t1.tv_sec;
	init_key[1] = t1.tv_usec;
	init_key[2] = getpid();

	tinymt64_init_by_array(&random_state, init_key, key_length);
}

/******************** MCA ARITHMETIC FUNCTIONS ********************
* The following set of functions perform the MCA operation. Operands
* are first converted to quad  format (GCC), inbound and outbound
* perturbations are applied using the _mca_inexact function, and the
* result converted to the original format for return
*******************************************************************/

// perform_bin_op: applies the binary operator (op) to (a) and (b)
// and stores the result in (res)
#define perform_bin_op(op, res, a, b)                               \
    switch (op){                                                    \
    case MCA_ADD: res=(a)+(b); break;                               \
    case MCA_MUL: res=(a)*(b); break;                               \
    case MCA_SUB: res=(a)-(b); break;                               \
    case MCA_DIV: res=(a)/(b); break;                               \
    default: perror("invalid operator in mcaquad.\n"); abort();     \
	};

static inline float _mca_sbin(float a, float b,const int  dop) {
	double da = (double)a;
	double db = (double)b;

	double res = 0;

	if (MCALIB_OP_TYPE != MCAMODE_RR) {
		_mca_inexactd(&da);
		_mca_inexactd(&db);
	}

    perform_bin_op(dop, res, da, db);

	if (MCALIB_OP_TYPE != MCAMODE_PB) {
		_mca_inexactd(&res);
	}

	return ((float)res);
}

static inline double _mca_dbin(double a, double b, const int qop) {
	__float128 qa = (__float128)a;
	__float128 qb = (__float128)b;
	__float128 res = 0;

	if (MCALIB_OP_TYPE != MCAMODE_RR) {
		_mca_inexactq(&qa);
		_mca_inexactq(&qb);
	}

    perform_bin_op(qop, res, qa, qb);

	if (MCALIB_OP_TYPE != MCAMODE_PB) {
		_mca_inexactq(&res);
	}

	return NEAREST_DOUBLE(res);

}

/************************* FPHOOKS FUNCTIONS *************************
* These functions correspond to those inserted into the source code
* during source to source compilation and are replacement to floating
* point operators
**********************************************************************/

static float _floatadd(float a, float b) {
	//return a + b
	return _mca_sbin(a, b, MCA_ADD);
}

static float _floatsub(float a, float b) {
	//return a - b
	return _mca_sbin(a, b, MCA_SUB);
}

static float _floatmul(float a, float b) {
	//return a * b
	return _mca_sbin(a, b, MCA_MUL);
}

static float _floatdiv(float a, float b) {
	//return a / b
	return _mca_sbin(a, b, MCA_DIV);
}


static double _doubleadd(double a, double b) {
	//return a + b
	return _mca_dbin(a, b, MCA_ADD);
}

static double _doublesub(double a, double b) {
	//return a - b
	return _mca_dbin(a, b, MCA_SUB);
}

static double _doublemul(double a, double b) {
	//return a * b
	return _mca_dbin(a, b, MCA_MUL);
}

static double _doublediv(double a, double b) {
	//return a / b
	return _mca_dbin(a, b, MCA_DIV);
}


struct mca_interface_t quad_mca_interface = {
	_floatadd,
	_floatsub,
	_floatmul,
	_floatdiv,
	_doubleadd,
	_doublesub,
	_doublemul,
	_doublediv,
	_mca_seed,
	_set_mca_mode,
	_set_mca_precision
};
