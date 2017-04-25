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
// 2015-11-16 New version using double precision for single precision operation
// 
// 2016-07-14 Support denormalized numbers
//
// 2017-04-25 Rewrite debug and validate the noise addition operation
//

#include <math.h>
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

static int 	MCALIB_OP_TYPE 		= MCAMODE_IEEE;
static int 	MCALIB_T		    = 53;

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
* perturbations used for MCA 
***************************************************************/

/* random generator internal state */
static tinymt64_t random_state;

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
  //complement the exponent, shift it at the right place in the MSW
  *x=( ((uint64_t) exp) + DOUBLE_EXP_COMP) << DOUBLE_PMAN_SIZE;
  res=*((double*)x);
  return res;
}

static inline uint32_t rexpq (__float128 x)
{
  //no need to check special value in our cases since qnoise will deal with it
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
	// test for minus infinity
	if (exp < -(QUAD_EXP_MIN+QUAD_PMAN_SIZE)) {
		SET_FLT128_WORDS64(noise, QMINF_hx, QMINF_lx);
		return noise;
	}
	//noise will be a subnormal
	//build HX with sign of d_rand, exp
	uint64_t u_hx=((uint64_t)(-QUAD_EXP_MIN + QUAD_EXP_COMP)) << QUAD_HX_PMAN_SIZE;
	//add the sign bit 
	uint64_t sign= u_rand&DOUBLE_GET_SIGN;
	u_hx=u_hx+sign;
	//erase the sign bit from u_rand
	u_rand=u_rand-sign;

	if (-exp-QUAD_EXP_MIN<-QUAD_HX_PMAN_SIZE){
		//the higher part of the noise start in HX of noise
		//set the mantissa part: U_rand>> by -exp-QUAD_EXP_MIN
		u_hx+=u_rand>>(-exp-QUAD_EXP_MIN+QUAD_EXP_SIZE+1/*SIGN_SIZE*/);
		//build LX with the remaining bits of the noise (-exp-QUAD_EXP_MIN-QUAD_HX_PMAN_SIZE) at the msb of LX
		//remove the bit already used in hx and put the remaining at msb of LX
		uint64_t u_lx=u_rand<<(QUAD_HX_PMAN_SIZE+exp+QUAD_EXP_MIN);	
        	SET_FLT128_WORDS64(noise, u_hx,u_lx );
	}else{ //the higher part of the noise start  in LX of noise
		//the noise as been already implicitly shifeted by QUAD_HX_PMAN_SIZE when starting in LX
		uint64_t u_lx=u_rand>>(-exp-QUAD_EXP_MIN-QUAD_HX_PMAN_SIZE);
		SET_FLT128_WORDS64(noise, u_hx, u_lx);
	}
	int prec = 20;
        int width = 46;
	//char buf[128];
	//int len=quadmath_snprintf (buf, sizeof(buf), "%+-#*.20Qe", width, noise);
	//if ((size_t) len < sizeof(buf))
        //printf ("subnormal noise %s\n", buf);
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
  int prec = 20;
        int width = 46;
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
	int32_t e_n = e_a - (MCALIB_T - 1);
	__float128 noise = qnoise(e_n);
	*qa=noise+*qa;
}

static int _mca_inexactd(double *da) {

	if (MCALIB_OP_TYPE == MCAMODE_IEEE) {
		return 0;
	}
	int32_t e_a=0;
	e_a=rexpd(*da);
	int32_t e_n = e_a - (MCALIB_T -1);
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
	double tmp=_mca_dbin(a,b,MCA_ADD);  
	return tmp;
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
