// The Bitmask Library - A tool for automated rounding error
// analysis of floating point software.
//
// Copyright (C) 2017 The Computer Engineering Laboratory, The
// University of Sydney. Maintained by Michael Frechtling:
// michael.frechtling@sydney.edu.au
//
// Copyright (C) 2017
//     Universite de Versailles St-Quentin-en-Yvelines                          *
//     CMLA, Ecole Normale Superieure de Cachan                                 *
//
// Changelog:
//
// 2015-05-20 replace random number generator with TinyMT64. This
// provides a reentrant, independent generator of better quality than
// the one provided in libc.
//
// 2015-11-14 remove effectless comparison functions, llvm will not 
// instrument it.
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

#include <math.h>
#include <mpfr.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdint.h>

#include "libbitmask.h"
#include "../vfcwrapper/vfcwrapper.h"
#include "../common/mca_const.h"
#include "../common/tinymt64.h"

static int 	BITMASKLIB_MODE         = BITMASK_MODE_ZERO;
static int 	MCALIB_T                = 53;

//possible op values
#define MCA_ADD 1
#define MCA_SUB 2
#define MCA_MUL 3
#define MCA_DIV 4

static float _bitmask_sbin(float a, float b, const int op);
static double _bitmask_dbin(double a, double b, const int op);

static uint32_t float_bitmask  = FLOAT_MASK_1;
static uint64_t double_bitmask = DOUBLE_MASK_1;

typedef union binary64_t {
  uint64_t h64;
  uint32_t h32[2];
}
binary64;

/* static uint8_t flip = 0; */

/******************** BITMASK CONTROL FUNCTIONS *******************
* The following functions are used to set virtual precision and
* BITMASK mode of operation.
***************************************************************/

static int _set_mca_mode(int mode){
  if (mode < 0 || mode > 3)
    return -1;
  
  BITMASKLIB_MODE = mode;
  return 0;
}

/* random generator internal state */
tinymt64_t random_state;

static int _set_mca_precision(int precision){
  MCALIB_T = precision;
  float_bitmask  = (MCALIB_T <= FLOAT_PREC)  ? (-1)    << (FLOAT_PREC  - MCALIB_T) : FLOAT_MASK_1;
  double_bitmask = (MCALIB_T <= DOUBLE_PREC) ? (-1ULL) << (DOUBLE_PREC - MCALIB_T) : DOUBLE_MASK_1;
  return 0;
}

/******************** BITMASK RANDOM FUNCTIONS ********************
* The following functions are used to calculate the random bitmask 
***************************************************************/

static uint64_t get_random_mask(void){
  return tinymt64_generate_uint64(&random_state);
}

static uint64_t get_random_dmask(void) {
  uint64_t mask = get_random_mask();
  return mask;
}

static uint32_t get_random_smask(void) {
  binary64 mask;
  mask.h64 = get_random_mask();
  /* flip ^= 1; */
  /* return mask.h32[flip]; */
  return mask.h32[0];
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

/******************** BITMASK ARITHMETIC FUNCTIONS ********************
* The following set of functions perform the BITMASK operation. Operands
* They apply a bitmask to the result
*******************************************************************/

// perform_bin_op: applies the binary operator (op) to (a) and (b)
// and stores the result in (res)
#define perform_bin_op(op, res, a, b)                               \
  switch (op){							    \
    case MCA_ADD: res=(a)+(b); break;                               \
    case MCA_MUL: res=(a)*(b); break;                               \
    case MCA_SUB: res=(a)-(b); break;                               \
    case MCA_DIV: res=(a)/(b); break;                               \
    default: perror("invalid operator in bitmask.\n"); abort();     \
  };

static float _bitmask_sbin(float a, float b, const int op) {
  float res = 0.0;
  uint32_t *tmp = (uint32_t*)&res;

  // Only RR mode is implemented
  perform_bin_op(op, res, a, b);

  uint32_t rand_smask = get_random_smask();

  if (BITMASKLIB_MODE == BITMASK_MODE_RAND) {
    /* Save higher bits than VERIFICARLO_PRECISION */
    uint32_t bits_saved = float_bitmask & (*tmp);
    /* Noise the result with the random mask by applying a XOR bitmask */
    uint32_t tmp_noised = *tmp ^ rand_smask;
    /* Remove the higher bits for restoring the bits saved by applying an OR bitmask */
    uint32_t tmp_with_bits_noised_only = tmp_noised & ~float_bitmask;
    /* Restore the saved bits by applying an OR bitmask */
    *tmp = tmp_with_bits_noised_only | bits_saved;
  }
  else if (BITMASKLIB_MODE == BITMASK_MODE_INV) 
    *tmp |= ~float_bitmask;
  else 
    *tmp &= float_bitmask;
  return NEAREST_FLOAT(res);
}

static double _bitmask_dbin(double a, double b, const int op) {
  double res = 0.0;
  uint64_t *tmp = (uint64_t*)&res;

  // Only RR mode is implemented
  perform_bin_op(op, res, a, b);

  uint64_t rand_dmask = get_random_dmask();

  if (BITMASKLIB_MODE == BITMASK_MODE_RAND) {
    /* Save higher bits than VERIFICARLO_PRECISION */
    uint64_t bits_saved = double_bitmask & (*tmp);
    /* Noise the result with the random mask by applying a XOR bitmask */
    uint64_t tmp_noised = *tmp ^ rand_dmask;
    /* Remove the higher bits for restoring the bits saved by applying an OR bitmask */
    uint64_t tmp_with_bits_noised_only = tmp_noised & ~double_bitmask;
    /* Restore the saved bits by applying an OR bitmask */
    *tmp = tmp_with_bits_noised_only | bits_saved;
  }
  else if (BITMASKLIB_MODE == BITMASK_MODE_INV)
    *tmp |= ~double_bitmask;
  else
    *tmp &= double_bitmask;
  return NEAREST_DOUBLE(res);
}

/******************** BITMASK COMPARE FUNCTIONS ********************
* Compare operations do not require BITMASK 
****************************************************************/

static int _floatgt(float a, float b) {
  // return a > b <-> a - b > 0
  return _bitmask_sbin(a, b, MCA_SUB) > 0;
}

static int _floatge(float a, float b) {
  // return a >= b <-> a - b >= 0
  return _bitmask_sbin(a, b, MCA_SUB) >= 0;
}

static int _floatlt(float a, float b) {
  // return a < b <-> a - b < 0
  return _bitmask_sbin(a, b, MCA_SUB) < 0;
}

static int _floatle(float a, float b) {
  // return a <= b <-> a - b <= 0
  return _bitmask_sbin(a, b, MCA_SUB) <= 0;
}

static int _doublegt(double a, double b) {
  // return a > b <-> a - b > 0
  return _bitmask_dbin(a, b, MCA_SUB) > 0;
}

static int _doublege(double a, double b) {
  // return a >= b <-> a - b >= 0
  return _bitmask_dbin(a, b, MCA_SUB) >= 0;
}

static int _doublelt(double a, double b) {
  // return a < b <-> a - b < 0
  return _bitmask_dbin(a, b, MCA_SUB) < 0;
}

static int _doublele(double a, double b) {
  // return a <= b <-> a - b <= 0
  return _bitmask_dbin(a, b, MCA_SUB) <= 0;
}

/************************* FPHOOKS FUNCTIONS *************************
* These functions correspond to those inserted into the source code
* during source to source compilation and are replacement to floating
* point operators
**********************************************************************/


static float _floatadd(float a, float b) {
	//return a + b
	return _bitmask_sbin(a, b, MCA_ADD);
}

static float _floatsub(float a, float b) {
	//return a - b
	return _bitmask_sbin(a, b, MCA_SUB);
}

static float _floatmul(float a, float b) {
	//return a * b
	return _bitmask_sbin(a, b, MCA_MUL);
}

static float _floatdiv(float a, float b) {
	//return a / b
	return _bitmask_sbin(a, b, MCA_DIV);
}


static double _doubleadd(double a, double b) {
	//return a + b
	return _bitmask_dbin(a, b, MCA_ADD);
}

static double _doublesub(double a, double b) {
	//return a - b
	return _bitmask_dbin(a, b, MCA_SUB);
}

static double _doublemul(double a, double b) {
	//return a * b
	return _bitmask_dbin(a, b, MCA_MUL);
}

static double _doublediv(double a, double b) {
	//return a / b
	return _bitmask_dbin(a, b, MCA_DIV);
}

struct mca_interface_t bitmask_interface = {
	_floatadd,
	_floatsub,
	_floatmul,
	_floatdiv,
        _floatgt,
	_floatge,
	_floatlt,
	_floatle,
	_doubleadd,
	_doublesub,
	_doublemul,
	_doublediv,
	_doublegt,
	_doublege,
	_doublelt,
	_doublele,
	_mca_seed,
	_set_mca_mode,
	_set_mca_precision
};
