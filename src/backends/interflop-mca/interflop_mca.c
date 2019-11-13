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
#include "../../common/float_struct.h"
#include "../../common/interflop.h"
#include "../../common/tinymt64.h"

typedef enum {
  KEY_PREC_B32,
  KEY_PREC_B64,
  KEY_MODE = 'm',
  KEY_SEED = 's'
} key_args;

typedef struct {
  bool choose_seed;
  uint64_t seed;
} t_context;

/* define the available MCA modes of operation */
typedef enum { ieee, mca, pb, rr } mcamode;

static const char *MCAMODE[] = {"ieee", "mca", "pb", "rr"};

/* define default environment variables and default parameters */
#define MCA_PRECISION_BINARY32_MIN 1
#define MCA_PRECISION_BINARY64_MIN 1
#define MCA_PRECISION_BINARY32_MAX 53
#define MCA_PRECISION_BINARY64_MAX 112
#define MCA_PRECISION_BINARY32_DEFAULT 24
#define MCA_PRECISION_BINARY64_DEFAULT 53
#define MCAMODE_DEFAULT mca

static int MCALIB_OP_TYPE = MCAMODE_DEFAULT;
static int MCALIB_BINARY32_T = MCA_PRECISION_BINARY32_DEFAULT;
static int MCALIB_BINARY64_T = MCA_PRECISION_BINARY64_DEFAULT;

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
  if (mode < ieee || mode > rr)
    return -1;

  MCALIB_OP_TYPE = mode;
  return 0;
}

static int _set_mca_precision_binary32(int precision) {
  if (precision < MCA_PRECISION_BINARY32_MIN) {
    errx(1, "interflop_mca: invalid precision for binary32 type. Must be "
            "greater than 0");
  } else if (precision > MCA_PRECISION_BINARY32_MAX) {
    warnx("interflop_mca: precision for binary32 type is too high, no noise "
          "will be added");
  } else {
    MCALIB_BINARY32_T = precision;
  }
  return 0;
}

static int _set_mca_precision_binary64(int precision) {
  if (precision < MCA_PRECISION_BINARY64_MIN) {
    errx(1, "interflop_mca: invalid precision for binary64 type. Must be "
            "greater than 0");
  } else if (precision > MCA_PRECISION_BINARY64_MAX) {
    warnx("interflop_mca: precision for binary64 type is too high, no noise "
          "will be added");
  } else {
    MCALIB_BINARY64_T = precision;
  }
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

static inline double pow2d(int exp) { return ldexp(1.0, exp); }

static inline int32_t rexpq(__float128 q) {
  binary128 x = {.f128 = q};
  return x.ieee.exponent - QUAD_EXP_COMP;
}

static inline int32_t rexpd(double d) {
  binary64 x = {.f64 = d};
  return x.ieee.exponent - DOUBLE_EXP_COMP;
}

/* noise = rand * 2^(exp) */
__float128 qnoise(int exp) {
  /* random number in (-0.5, 0.5) */
  const binary64 brand = {.f64 = _mca_rand() - 0.5};
  const int32_t brand_exp = brand.ieee.exponent - DOUBLE_EXP_COMP;
  const int32_t noise_exp = brand_exp + exp;

  binary128 noise;

  /* specials */
  if (exp == 0) {
    noise.f128 = brand.f64;
  }
  /* We exceed the maximal exponent */
  /* we return an sign(rand) * Infinity */
  else if (noise_exp > QUAD_EXP_MAX) {
    noise.ieee128.sign = brand.ieee.sign;
    noise.ieee128.exponent = QUAD_EXP_MAX;
    noise.ieee128.mantissa = 0;
  }
  /* We are in the subnormal case */
  else if (noise_exp < -QUAD_EXP_MIN) {
    /* test if result is below the minimal subnormal quad number */
    if (noise_exp < -(QUAD_EXP_MIN + QUAD_PMAN_SIZE)) {
      noise.f128 = 0.0;
    } else {
      /* noise will be a subnormal */
      /* add the sign bit  */
      noise.ieee128.sign = brand.ieee.sign;
      /* set the exponent to 0 (subnormal unbiased exponent) */
      noise.ieee128.exponent = 0;
      /* set the noise mantissa to the rand mantissa */
      noise.ieee128.mantissa = brand.ieee.mantissa;
      /* we set a 52bits to a 112bits so we need to */
      /* scale the mantissa at the MSD */
      noise.ieee128.mantissa <<= QUAD_PMAN_SIZE - DOUBLE_PMAN_SIZE;
      /* we are in the subnormal case, so we need to shift the mantissa to the
       * right place */
      noise.ieee128.mantissa >>= -noise_exp - QUAD_EXP_MIN;
    }
  }
  /* normal case */
  else {
    /* set sign = sign of rand */
    noise.ieee128.sign = brand.ieee.sign;
    /* set the exponent = exp + exp_rand + BIAS */
    noise.ieee128.exponent = noise_exp + QUAD_EXP_COMP;
    /* set the noise mantissa to the rand mantissa */
    noise.ieee128.mantissa = brand.ieee.mantissa;
    /* we set a 52bits to a 112bits so we need to */
    /* scale the mantissa at the MSD */
    noise.ieee128.mantissa <<= (QUAD_PMAN_SIZE - DOUBLE_PMAN_SIZE);
  }

  return noise.f128;
}

static bool _is_representableq(__float128 *qa) {

  /* Check if *qa is exactly representable
   * in the current virtual precision */
  binary128 b128 = {.f128 = *qa};
  uint64_t hx = b128.ieee.mant_high, lx = b128.ieee.mant_low;

  /* compute representable bits in hx and lx */
  char bits_in_hx = min((MCALIB_BINARY64_T - 1), QUAD_HX_PMAN_SIZE);
  char bits_in_lx = (MCALIB_BINARY64_T - 1) - bits_in_hx;

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
  return ((p_mantissa << (MCALIB_BINARY32_T + DOUBLE_EXP_SIZE)) == 0);
}

static int _mca_inexactq(__float128 *qa) {

  if (MCALIB_OP_TYPE == ieee) {
    return 0;
  }

  /* Checks that we are not in a special cases */
  if (fpclassifyq(*qa) != FP_NORMAL && fpclassifyq(*qa) != FP_SUBNORMAL) {
    return 0;
  }

  /* In RR if the number is representable in current virtual precision,
   * do not add any noise */
  if (MCALIB_OP_TYPE == rr && _is_representableq(qa)) {
    return 0;
  }

  int32_t e_a = 0;
  e_a = rexpq(*qa);
  int32_t e_n = e_a - (MCALIB_BINARY64_T - 1);
  __float128 noise = qnoise(e_n);
  *qa = *qa + noise;
}

static int _mca_inexactd(double *da) {

  if (MCALIB_OP_TYPE == ieee) {
    return 0;
  }

  /* Checks that we are not in a special cases */
  if (fpclassify(*da) != FP_NORMAL && fpclassify(*da) != FP_SUBNORMAL) {
    return 0;
  }

  /* In RR if the number is representable in current virtual precision,
   * do not add any noise */
  if (MCALIB_OP_TYPE == rr && _is_representabled(da)) {
    return 0;
  }

  int32_t e_a = 0;
  binary64 ba = {.f64 = *da};
  e_a = rexpd(*da);
  int32_t e_n = e_a - (MCALIB_BINARY32_T - 1);
  double d_rand = (_mca_rand() - 0.5);
  *da = *da + pow2d(e_n) * d_rand;
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

static inline float _mca_binary32_binary_op(float a, float b, const int dop) {
  double da = (double)a;
  double db = (double)b;

  double res = 0;

  if (MCALIB_OP_TYPE != rr) {
    _mca_inexactd(&da);
    _mca_inexactd(&db);
  }

  perform_bin_op(dop, res, da, db);

  if (MCALIB_OP_TYPE != pb) {
    _mca_inexactd(&res);
  }

  return ((float)res);
}

static inline double _mca_binary64_binary_op(double a, double b,
                                             const int qop) {
  __float128 qa = (__float128)a;
  __float128 qb = (__float128)b;
  __float128 res = 0;

  if (MCALIB_OP_TYPE != rr) {
    _mca_inexactq(&qa);
    _mca_inexactq(&qb);
  }

  perform_bin_op(qop, res, qa, qb);

  if (MCALIB_OP_TYPE != pb) {
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
  *c = _mca_binary32_binary_op(a, b, MCA_ADD);
}

static void _interflop_sub_float(float a, float b, float *c, void *context) {
  *c = _mca_binary32_binary_op(a, b, MCA_SUB);
}

static void _interflop_mul_float(float a, float b, float *c, void *context) {
  *c = _mca_binary32_binary_op(a, b, MCA_MUL);
}

static void _interflop_div_float(float a, float b, float *c, void *context) {
  *c = _mca_binary32_binary_op(a, b, MCA_DIV);
}

static void _interflop_add_double(double a, double b, double *c,
                                  void *context) {
  *c = _mca_binary64_binary_op(a, b, MCA_ADD);
}

static void _interflop_sub_double(double a, double b, double *c,
                                  void *context) {
  *c = _mca_binary64_binary_op(a, b, MCA_SUB);
}

static void _interflop_mul_double(double a, double b, double *c,
                                  void *context) {
  *c = _mca_binary64_binary_op(a, b, MCA_MUL);
}

static void _interflop_div_double(double a, double b, double *c,
                                  void *context) {
  *c = _mca_binary64_binary_op(a, b, MCA_DIV);
}

static struct argp_option options[] = {
    /* --debug, sets the variable debug = true */
    {"precision-binary32", KEY_PREC_B32, "PRECISION", 0,
     "select precision for binary32 (PRECISION >= 0)"},
    {"precision-binary64", KEY_PREC_B64, "PRECISION", 0,
     "select precision for binary64 (PRECISION >= 0)"},
    {"mode", KEY_MODE, "MODE", 0, "select MCA mode among {ieee, mca, pb, rr}"},
    {"seed", KEY_SEED, "SEED", 0, "fix the random generator seed"},
    {0}};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  t_context *ctx = (t_context *)state->input;
  char *endptr;
  int val = -1;
  switch (key) {
  case KEY_PREC_B32:
    /* precision for binary32 */
    errno = 0;
    val = strtol(arg, &endptr, 10);
    if (errno != 0 || val <= 0) {
      errx(1, "interflop_mca: --precision-binary32 invalid value provided, "
              "must be a "
              "positive integer.");
    } else {
      _set_mca_precision_binary32(val);
    }
    break;
  case KEY_PREC_B64:
    /* precision for binary64 */
    errno = 0;
    val = strtol(arg, &endptr, 10);
    if (errno != 0 || val <= 0) {
      errx(1, "interflop_mca: --precision-binary64 invalid value provided, "
              "must be a "
              "positive integer.");
    } else {
      _set_mca_precision_binary64(val);
    }
    break;
  case KEY_MODE:
    /* mode */
    if (strcasecmp(MCAMODE[ieee], arg) == 0) {
      _set_mca_mode(ieee);
    } else if (strcasecmp(MCAMODE[mca], arg) == 0) {
      _set_mca_mode(mca);
    } else if (strcasecmp(MCAMODE[pb], arg) == 0) {
      _set_mca_mode(pb);
    } else if (strcasecmp(MCAMODE[rr], arg) == 0) {
      _set_mca_mode(rr);
    } else {
      errx(1, "interflop_mca: --mode invalid value provided, must be one of: "
              "{ieee, mca, pb, rr}.");
    }
    break;
  case KEY_SEED:
    errno = 0;
    ctx->choose_seed = true;
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
  ctx->choose_seed = false;
  ctx->seed = 0ULL;
}

struct interflop_backend_interface_t interflop_init(int argc, char **argv,
                                                    void **context) {

  _set_mca_precision_binary32(MCA_PRECISION_BINARY32_DEFAULT);
  _set_mca_precision_binary64(MCA_PRECISION_BINARY64_DEFAULT);
  _set_mca_mode(MCAMODE_DEFAULT);

  t_context *ctx = malloc(sizeof(t_context));
  *context = ctx;
  init_context(ctx);

  /* parse backend arguments */
  argp_parse(&argp, argc, argv, 0, 0, ctx);

  warnx("interflop_mca: loaded backend with precision-binary32 = %d, "
        "precision-binary64 = %d and mode = %s",
        MCALIB_BINARY32_T, MCALIB_BINARY64_T, MCAMODE[MCALIB_OP_TYPE]);

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
