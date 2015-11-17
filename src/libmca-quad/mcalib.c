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
// 2015-05-20 New version based on quad flotting point type to replace MPFR until
// required MCA precision is lower than quad mantissa divided by 2, i.e. 56 bits
        //can we use bits manipulation instead of qmul?
//

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


#define NEAREST_FLOAT(x)	((float) (x))
#define	NEAREST_DOUBLE(x)	((double) (x))

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

#define QINF_hx 0x7fff000000000000ULL
#define QINF_lx 0x0000000000000000ULL

static __float128 pow2q(int exp) {
  __float128 res=0;
  uint64_t hx, lx;

  //specials
  if (exp == 0) return 1;

  if (exp > 16383) { /*exceed max exponent*/
	SET_FLT128_WORDS64(res, QINF_hx, QINF_lx);
	return res;
  }
  if (exp < -16382) { /*subnormal*/
	if (exp+16383<-48)
        	SET_FLT128_WORDS64(res, ((uint64_t) 0 ) , (0x8000000000000000ULL) >> -(exp+16383));
	else
		SET_FLT128_WORDS64(res, ((uint64_t) 0x0000800000000000ULL ) >> -(exp+16383+64) , ((uint64_t) 0 ));
	return res;
  }

  //normal case
  //complement the exponent, sift it at the right place in the MSW
  hx=( ((uint64_t) exp) + 16382) << 48;
  lx=0;
  SET_FLT128_WORDS64(res, hx, lx);
  return res;
}

static double pow2d(int exp) {
  double res=0;
  uint64_t *x;

  //specials
  if (exp == 0) return 1;

  if (exp > 1023) { /*exceed max exponent*/
	*x= 0x7FF0000000000000ULL;
  	res=*((double*)x);
	return res;
  }
  if (exp < -1022) { /*subnormal*/
        *x=((uint64_t) 0x0008000000000000ULL ) >> -(exp+1023);
	res=*((double*)x);
        return res;
}

  //normal case
  //complement the exponent, sift it at the right place in the MSW
  *x=( ((uint64_t) exp) + 1022) << 52;
  res=*((double*)x);
  return res;
}

static uint32_t rexpq (__float128 x)
{
  //no need to check special value in our cases since pow2q will deal with it
  //do not reuse it outside this code!
  uint64_t hx,ix;
  uint32_t exp=0;
  GET_FLT128_MSW64(hx,x);
  //remove sign bit, mantissa will be erased by the next shift
  ix = hx&0x7fffffffffffffffULL;
  //shift exponent to have LSB on position 0 and complement
  exp += (ix>>48)-16382;
  return exp;
}

static uint32_t rexpd (double x)
{
  //no need to check special value in our cases since pow2d will deal with it
  //do not reuse it outside this code!
  uint64_t *phex,hex,ix;
  phex=&hex;
  uint32_t exp=0;
  *phex=*((uint64_t*) &x);
  //remove sign bit, mantissa will be erased by the next shift
  ix = (*phex)&0x7fffffffffffffffULL;
  //shift exponent to have LSB on position 0 and complement
  exp += (ix>>52)-1022;
  return exp;
}

__float128 qnoise(int exp, double d_rand){
  //double d_rand = (_mca_rand() - 0.5);
  uint64_t u_rand= *((uint64_t*) &d_rand);
  //pow2q should be inlined by hand to avoid useless convertion and fct calls
  __float128 noise=pow2q(exp);
  uint64_t hx=0;
  uint64_t lx=0;
  GET_FLT128_MSW64(hx,noise);
  //set sign = sign of d_rand
  hx+=u_rand&0x8000000000000000ULL;
  //extract u_rand (pseudo) mantissa and put the first 48 bits in hx...
  uint64_t p_mantissa=u_rand&0x000fffffffffffffULL;
  hx+=(p_mantissa)>>4;//4=52 (double pmantissa) - 48
  //...and the last 4 in lx at msb
  lx+=(p_mantissa)<<60;//60=1(s)+11(exp double)+48(hx)
  SET_FLT128_WORDS64(noise, hx, lx);
  return noise;
}

static int _mca_inexactq(__float128 *qa) {

	if (MCALIB_OP_TYPE == MCAMODE_IEEE) {
		return 0;
	}

	//shall we remove it to remove the if for all other values?
	//1% improvment on kahan => is better or worst on other benchmarks?
	//if (qa == 0) {
	//	return 0;
	//}

	int32_t e_a=0;
	e_a=rexpq(*qa);
	int32_t e_n = e_a - (MCALIB_T - 1);
	double d_rand = (_mca_rand() - 0.5);
	//can we use bits manipulation instead of qmul?
	__float128 noise = qnoise(e_n, d_rand);
	*qa=noise+*qa;
    //printf(" noise 1 = %.17g, noise 2 = %.17g\n", NEAREST_DOUBLE(noise), NEAREST_DOUBLE(*qa));
}

static int _mca_inexactd(double *da) {

	if (MCALIB_OP_TYPE == MCAMODE_IEEE) {
		return 0;
	}
	int32_t e_a=0;
	//use of standard frexp allows to check custom rexpd output
	//frexp (*da, &e_a);
	//printf("exp of a = %d\n",e_a);
	e_a=rexpd(*da);
	int32_t e_n = e_a - (MCALIB_T - 1);
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

static float _mca_sbin(float a, float b,int  dop) {
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

static double _mca_dbin(double a, double b, int qop) {
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
