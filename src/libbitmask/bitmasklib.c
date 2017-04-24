// The Monte Carlo Arihmetic Library - A tool for automated rounding error
// analysis of floating point software.
//
// Copyright (C) 2014 The Computer Engineering Laboratory, The
// University of Sydney. Maintained by Michael Frechtling:
// michael.frechtling@sydney.edu.au
//
// Copyright (C) 2015
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
#include "random_bitarray.h"

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
static uint64_t index_bitarray = 0;

/******************** MCA CONTROL FUNCTIONS *******************
* The following functions are used to set virtual precision and
* MCA mode of operation.
***************************************************************/

static int _set_mca_mode(int mode){
  if (mode < 0 || mode > 2)
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
  index_bitarray = tinymt64_generate_uint64(&random_state) % (SIZE_BITARRAY / sizeof(uint64_t));
  return 0;
}

/******************** MCA RANDOM FUNCTIONS ********************
* The following functions are used to calculate the random
* perturbations used for MCA and apply these to MPFR format
* operands
***************************************************************/

static uint32_t get_random_smask(void){
  uint32_t *ptr_32 = (uint32_t*)&random_bitarray;
  uint32_t smask = ptr_32[index_bitarray]; 
  index_bitarray = (index_bitarray+1) % (SIZE_BITARRAY / sizeof(uint32_t));
  return smask;
}

static uint64_t get_random_dmask(void) { 
  uint64_t *ptr_64 = (uint64_t*)&random_bitarray;
  uint64_t dmask = ptr_64[index_bitarray]; 
  index_bitarray = (index_bitarray+1) % (SIZE_BITARRAY / sizeof(uint64_t));
  return dmask;
}

static void _mca_seed(void) {
  return;
}

/******************** MCA ARITHMETIC FUNCTIONS ********************
* The following set of functions perform the MCA operation. Operands
* are first converted to MPFR format, inbound and outbound perturbations
* are applied using the _mca_inexact function, and the result converted
* to the original format for return
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
  perform_bin_op(op, res, a, b);
  uint32_t rand_smask = get_random_smask();
  if (BITMASKLIB_MODE == BITMASK_MODE_RAND) {
    /* Save bits higher than VERIFICARLO_PRECISION */
    uint32_t bits_saved = float_bitmask & (*tmp);
    /* Noise result with the random mask by apply a XOR */
    uint32_t tmp_noised = *tmp ^ rand_smask;
    /* Remove higher bits to restore bits saved by apply an OR */
    uint32_t tmp_with_bits_noised_only = tmp_noised & ~float_bitmask;
    /* Restore saved bits by apply an OR */
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
  perform_bin_op(op, res, a, b);
  uint64_t rand_dmask = get_random_dmask();
  if (BITMASKLIB_MODE == BITMASK_MODE_RAND) {
    /* Save bits higher than VERIFICARLO_PRECISION */
    uint64_t bits_saved = double_bitmask & (*tmp);
    /* Noise result with the random mask by apply a XOR */
    uint64_t tmp_noised = *tmp ^ rand_dmask;
    /* Remove higher bits to restore bits saved by apply an OR */
    uint64_t tmp_with_bits_noised_only = tmp_noised & ~double_bitmask;
    /* Restore saved bits by apply an OR */
    *tmp = tmp_with_bits_noised_only | bits_saved;
  }
  else if (BITMASKLIB_MODE == BITMASK_MODE_INV)
    *tmp |= ~double_bitmask;
  else
    *tmp &= double_bitmask;
  return NEAREST_DOUBLE(res);
}

/******************** MCA COMPARE FUNCTIONS ********************
* Compare operations do not require MCA 
****************************************************************/


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
	_doubleadd,
	_doublesub,
	_doublemul,
	_doublediv,
	_mca_seed,
	_set_mca_mode,
	_set_mca_precision
};
