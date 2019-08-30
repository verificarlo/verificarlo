/*****************************************************************************
 *                                                                           *
 *  This file is part of Verificarlo.                                        *
 *                                                                           *
 *  Copyright (c) 2015                                                       *
 *     Universite de Versailles St-Quentin-en-Yvelines                       *
 *     CMLA, Ecole Normale Superieure de Cachan                              *
 *  Copyright (c) 2018                                                       *
 *     Universite de Versailles St-Quentin-en-Yvelines                       *
 *                                                                           *
 *  Verificarlo is free software: you can redistribute it and/or modify      *
 *  it under the terms of the GNU General Public License as published by     *
 *  the Free Software Foundation, either version 3 of the License, or        *
 *  (at your option) any later version.                                      *
 *                                                                           *
 *  Verificarlo is distributed in the hope that it will be useful,           *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *  GNU General Public License for more details.                             *
 *                                                                           *
 *  You should have received a copy of the GNU General Public License        *
 *  along with Verificarlo.  If not, see <http://www.gnu.org/licenses/>.     *
 *                                                                           *
 *****************************************************************************/

// Changelog:
//
// 2015-05-20 replace random number generator with TinyMT64. This
// provides a reentrant, independent generator of better quality than
// the one provided in libc.
//
// 2015-10-11 New version based on quad floating point type to replace MPFR
// until
// required MCA precision is lower than quad mantissa divided by 2, i.e. 56 bits
//
// 2015-11-16 New version using double precision for single precision operation
//
// 2016-07-14 Support denormalized numbers
//
// 2017-04-25 Rewrite debug and validate the noise addition operation
//
// 2019-08-07 Fix memory leak and convert to interflop

#include <argp.h>
#include <err.h>
#include <errno.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/time.h>
#include <unistd.h>

#include "../../common/float_const.h"
#include "../../common/interflop.h"
#include "../../common/tinymt64.h"

#include "quadmath-imp.h"

typedef struct {
  int choose_seed;
  uint64_t seed;
} t_context;

/* define the available MCA modes of operation */
#define MCAMODE_IEEE 0
#define MCAMODE_MCA 1
#define MCAMODE_PB 2
#define MCAMODE_RR 3

static const char *MCAMODE[] = {"ieee", "mca", "pb", "rr"};

/* define default environment variables and default parameters */
#define MCA_PRECISION_DEFAULT 53
#define MCAMODE_DEFAULT MCAMODE_MCA

static int MCALIB_OP_TYPE = MCAMODE_DEFAULT;
static int MCALIB_T = MCA_PRECISION_DEFAULT;

// possible op values
#define MCA_ADD 1
#define MCA_SUB 2
#define MCA_MUL 3
#define MCA_DIV 4

#define min(a, b) ((a) < (b) ? (a) : (b))

static float _mca_sbin(float a, float b, int qop);

static double _mca_dbin(double a, double b, int qop);

/******************** MCA CONTROL FUNCTIONS *******************
 * The following functions are used to set virtual precision and
 * MCA mode of operation.
 ***************************************************************/

static int _set_mca_mode(int mode) {
  if (mode < 0 || mode > 3)
    return -1;

  MCALIB_OP_TYPE = mode;
  return 0;
}

static int _set_mca_precision(int precision) {
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
  double res = 0;
  uint64_t x[1];
  // specials
  if (exp == 0)
    return 1;

  if (exp > 1023) { /*exceed max exponent*/
    *x = DOUBLE_PLUS_INF;
    res = *((double *)x);
    return res;
  }
  if (exp < -1022) { /*subnormal*/
    *x = ((uint64_t)DOUBLE_PMAN_MSB) >> -(exp + DOUBLE_EXP_MAX);
    res = *((double *)x);
    return res;
  }

  // normal case
  // complement the exponent, shift it at the right place in the MSW
  *x = (((uint64_t)exp) + DOUBLE_EXP_COMP) << DOUBLE_PMAN_SIZE;
  res = *((double *)x);
  return res;
}

static inline uint32_t rexpq(__float128 x) {
  // no need to check special value in our cases since qnoise will deal with it
  // do not reuse it outside this code!
  uint64_t hx, ix;
  uint32_t exp = 0;
  GET_FLT128_MSW64(hx, x);
  // remove sign bit, mantissa will be erased by the next shift
  ix = hx & QUAD_HX_ERASE_SIGN;
  // shift exponent to have LSB on position 0 and complement
  exp += (ix >> QUAD_HX_PMAN_SIZE) - QUAD_EXP_COMP;
  return exp;
}

static inline uint32_t rexpd(double x) {
  // no need to check special value in our cases since pow2d will deal with it
  // do not reuse it outside this code!
  uint64_t hex, ix;
  uint32_t exp = 0;
  // change type to bit field
  hex = *((uint64_t *)&x);
  // remove sign bit, mantissa will be erased by the next shift
  ix = hex & DOUBLE_ERASE_SIGN;
  // shift exponent to have LSB on position 0 and complement
  exp += (ix >> DOUBLE_PMAN_SIZE) - DOUBLE_EXP_COMP;
  return exp;
}

/* Returns the MCA noise for the quad format
 * qnoise = 2^(exp)*d_rand */
static inline __float128 qnoise(int exp) {
  double d_rand = (_mca_rand() - 0.5);
  uint64_t u_rand = *((uint64_t *)&d_rand);
  __float128 noise;
  uint64_t hx, lx;

  if (exp > QUAD_EXP_MAX) { /*exceed max exponent*/
    SET_FLT128_WORDS64(noise, QINF_hx, QINF_lx);
    return noise;
  }
  if (exp < -QUAD_EXP_MIN) { /*subnormal*/
    // test for minus infinity
    if (exp < -(QUAD_EXP_MIN + QUAD_PMAN_SIZE)) {
      SET_FLT128_WORDS64(noise, QMINF_hx, QMINF_lx);
      return noise;
    }
    // noise will be a subnormal
    // build HX with sign of d_rand, exp
    uint64_t u_hx = ((uint64_t)(-QUAD_EXP_MIN + QUAD_EXP_COMP))
                    << QUAD_HX_PMAN_SIZE;
    // add the sign bit
    uint64_t sign = u_rand & DOUBLE_GET_SIGN;
    u_hx = u_hx + sign;
    // erase the sign bit from u_rand
    u_rand = u_rand - sign;

    if (-exp - QUAD_EXP_MIN < -QUAD_HX_PMAN_SIZE) {
      // the higher part of the noise start in HX of noise
      // set the mantissa part: U_rand>> by -exp-QUAD_EXP_MIN
      u_hx += u_rand >> (-exp - QUAD_EXP_MIN + QUAD_EXP_SIZE + 1 /*SIGN_SIZE*/);
      // build LX with the remaining bits of the noise
      // (-exp-QUAD_EXP_MIN-QUAD_HX_PMAN_SIZE) at the msb of LX
      // remove the bit already used in hx and put the remaining at msb of LX
      uint64_t u_lx = u_rand << (QUAD_HX_PMAN_SIZE + exp + QUAD_EXP_MIN);
      SET_FLT128_WORDS64(noise, u_hx, u_lx);
    } else { // the higher part of the noise start  in LX of noise
      // the noise as been already implicitly shifeted by QUAD_HX_PMAN_SIZE when
      // starting in LX
      uint64_t u_lx = u_rand >> (-exp - QUAD_EXP_MIN - QUAD_HX_PMAN_SIZE);
      SET_FLT128_WORDS64(noise, u_hx, u_lx);
    }
    // char buf[128];
    // int len=quadmath_snprintf (buf, sizeof(buf), "%+-#*.20Qe", width, noise);
    // if ((size_t) len < sizeof(buf))
    // printf ("subnormal noise %s\n", buf);
    return noise;
  }
  // normal case
  // complement the exponent, shift it at the right place in the MSW
  hx = (((uint64_t)exp + rexpd(d_rand)) + QUAD_EXP_COMP) << QUAD_HX_PMAN_SIZE;
  // set sign = sign of d_rand
  hx += u_rand & DOUBLE_GET_SIGN;
  // extract u_rand (pseudo) mantissa and put the first 48 bits in hx...
  uint64_t p_mantissa = u_rand & DOUBLE_GET_PMAN;
  hx += (p_mantissa) >>
        (DOUBLE_PMAN_SIZE - QUAD_HX_PMAN_SIZE); // 4=52 (double pmantissa) - 48
  //...and the last 4 in lx at msb
  // uint64_t
  lx = (p_mantissa) << (SIGN_SIZE + DOUBLE_EXP_SIZE +
                        QUAD_HX_PMAN_SIZE); // 60=1(s)+11(exp double)+48(hx)
  SET_FLT128_WORDS64(noise, hx, lx);
  return noise;
}

static bool _is_representableq(__float128 *qa) {
  /* Check if *qa is exactly representable
   * in the current virtual precision */
  uint64_t hx, lx;
  GET_FLT128_WORDS64(hx, lx, *qa);

  /* compute representable bits in hx and lx */
  char bits_in_hx = min((MCALIB_T - 1), QUAD_HX_PMAN_SIZE);
  char bits_in_lx = (MCALIB_T - 1) - bits_in_hx;

  /* check bits in lx */
  /* here we know that bits_in_lx < 64 */
  bool representable = ((lx << bits_in_lx) == 0);

  /* check bits in hx,
   * the test always succeeds when bits_in_hx == QUAD_HX_PMAN_SIZE,
   * cannot remove the test since << 64 is undefined in C. */
  if (bits_in_hx < QUAD_HX_PMAN_SIZE) {
    representable &= ((hx << (1 + QUAD_EXP_SIZE + bits_in_hx)) == 0);
  }

  return representable;
}

static bool _is_representabled(double *da) {
  /* Check if *da is exactly representable
   * in the current virtual precision */
  uint64_t p_mantissa = (*((uint64_t *)da)) & DOUBLE_GET_PMAN;
  /* here we know that (MCALIB_T-1) < 53 */
  return ((p_mantissa << (MCALIB_T - 1)) == 0);
}

static int _mca_inexactq(__float128 *qa) {
  if (MCALIB_OP_TYPE == MCAMODE_IEEE) {
    return 0;
  }

  /* In RR if the number is representable in current virtual precision,
   * do not add any noise */
  if (MCALIB_OP_TYPE == MCAMODE_RR && _is_representableq(qa)) {
    return 0;
  }

  int32_t e_a = 0;
  e_a = rexpq(*qa);
  int32_t e_n = e_a - (MCALIB_T - 1);
  __float128 noise = qnoise(e_n);
  *qa = noise + *qa;
  return 1;
}

static int _mca_inexactd(double *da) {
  if (MCALIB_OP_TYPE == MCAMODE_IEEE) {
    return 0;
  }

  /* In RR if the number is representable in current virtual precision,
   * do not add any noise */
  if (MCALIB_OP_TYPE == MCAMODE_RR && _is_representabled(da)) {
    return 0;
  }

  int32_t e_a = 0;
  e_a = rexpd(*da);
  int32_t e_n = e_a - (MCALIB_T - 1);
  double d_rand = (_mca_rand() - 0.5);
  *da = *da + pow2d(e_n) * d_rand;
  return 1;
}

static void _set_mca_seed(int choose_seed, uint64_t seed) {
  if (choose_seed) {
    tinymt64_init(&random_state, seed);
  } else {
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
}

/******************** MCA ARITHMETIC FUNCTIONS ********************
 * The following set of functions perform the MCA operation. Operands
 * are first converted to quad  format (GCC), inbound and outbound
 * perturbations are applied using the _mca_inexact function, and the
 * result converted to the original format for return
 *******************************************************************/

// perform_bin_op: applies the binary operator (op) to (a) and (b)
// and stores the result in (res)
#define perform_bin_op(op, res, a, b)                                          \
  switch (op) {                                                                \
  case MCA_ADD:                                                                \
    res = (a) + (b);                                                           \
    break;                                                                     \
  case MCA_MUL:                                                                \
    res = (a) * (b);                                                           \
    break;                                                                     \
  case MCA_SUB:                                                                \
    res = (a) - (b);                                                           \
    break;                                                                     \
  case MCA_DIV:                                                                \
    res = (a) / (b);                                                           \
    break;                                                                     \
  default:                                                                     \
    perror("invalid operator in mcaquad.\n");                                  \
    abort();                                                                   \
  };

static inline float _mca_sbin(float a, float b, const int dop) {
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

static void _interflop_add_float(float a, float b, float *c, void *context) {
  *c = _mca_sbin(a, b, MCA_ADD);
}

static void _interflop_sub_float(float a, float b, float *c, void *context) {
  *c = _mca_sbin(a, b, MCA_SUB);
}

static void _interflop_mul_float(float a, float b, float *c, void *context) {
  *c = _mca_sbin(a, b, MCA_MUL);
}

static void _interflop_div_float(float a, float b, float *c, void *context) {
  *c = _mca_sbin(a, b, MCA_DIV);
}

static void _interflop_add_double(double a, double b, double *c,
                                  void *context) {
  *c = _mca_dbin(a, b, MCA_ADD);
}

static void _interflop_sub_double(double a, double b, double *c,
                                  void *context) {
  *c = _mca_dbin(a, b, MCA_SUB);
}

static void _interflop_mul_double(double a, double b, double *c,
                                  void *context) {
  *c = _mca_dbin(a, b, MCA_MUL);
}

static void _interflop_div_double(double a, double b, double *c,
                                  void *context) {
  *c = _mca_dbin(a, b, MCA_DIV);
}

static struct argp_option options[] = {
    /* --debug, sets the variable debug = true */
    {"precision", 'p', "PRECISION", 0, "select precision (PRECISION >= 0)"},
    {"mode", 'm', "MODE", 0, "select MCA mode among {ieee, mca, pb, rr}"},
    {"seed", 's', "SEED", 0, "fix the random generator seed"},
    {0}};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  t_context *ctx = (t_context *)state->input;
  char *endptr;
  switch (key) {
  case 'p':
    /* precision */
    errno = 0;
    int val = strtol(arg, &endptr, 10);
    if (errno != 0 || val <= 0) {
      errx(1, "interflop_mca: --precision invalid value provided, must be a "
              "positive integer.");
    } else {
      _set_mca_precision(val);
    }
    break;
  case 'm':
    /* mode */
    if (strcasecmp(MCAMODE[MCAMODE_IEEE], arg) == 0) {
      _set_mca_mode(MCAMODE_IEEE);
    } else if (strcasecmp(MCAMODE[MCAMODE_MCA], arg) == 0) {
      _set_mca_mode(MCAMODE_MCA);
    } else if (strcasecmp(MCAMODE[MCAMODE_PB], arg) == 0) {
      _set_mca_mode(MCAMODE_PB);
    } else if (strcasecmp(MCAMODE[MCAMODE_RR], arg) == 0) {
      _set_mca_mode(MCAMODE_RR);
    } else {
      errx(1, "interflop_mca: --mode invalid value provided, must be one of: "
              "{ieee, mca, pb, rr}.");
    }
    break;
  case 's':
    errno = 0;
    ctx->choose_seed = 1;
    ctx->seed = strtoull(arg, &endptr, 10);
    if (errno != 0) {
      errx(1,
           "interflop_mca: --seed invalid value provided, must be an integer");
    }
    break;
  default:
    return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

static struct argp argp = {options, parse_opt, "", ""};

static void init_context(t_context *ctx) {
  ctx->choose_seed = 0;
  ctx->seed = 0ULL;
}

struct interflop_backend_interface_t interflop_init(int argc, char **argv,
                                                    void **context) {

  _set_mca_precision(MCA_PRECISION_DEFAULT);
  _set_mca_mode(MCAMODE_DEFAULT);

  t_context *ctx = malloc(sizeof(t_context));
  *context = ctx;
  init_context(ctx);

  /* parse backend arguments */
  argp_parse(&argp, argc, argv, 0, 0, ctx);

  warnx("interflop_mca: loaded backend with precision = %d and mode = %s",
        MCALIB_T, MCAMODE[MCALIB_OP_TYPE]);

  struct interflop_backend_interface_t interflop_backend_mca = {
      _interflop_add_float,
      _interflop_sub_float,
      _interflop_mul_float,
      _interflop_div_float,
      NULL,
      _interflop_add_double,
      _interflop_sub_double,
      _interflop_mul_double,
      _interflop_div_double,
      NULL};

  /* Initialize the seed */
  _set_mca_seed(ctx->choose_seed, ctx->seed);

  return interflop_backend_mca;
}
