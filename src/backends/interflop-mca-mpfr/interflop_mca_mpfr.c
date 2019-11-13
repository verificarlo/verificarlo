// Interflop Monte Carlo Arithmetic MPFR Backend
//
// Copyright (C) 2014 The Computer Engineering Laboratory, The
// University of Sydney. Maintained by Michael Frechtling:
// michael.frechtling@sydney.edu.au
//
// Copyright (C) 2015
//     Universite de Versailles St-Quentin-en-Yvelines
//     CMLA, Ecole Normale Superieure de Cachan
//
// Copyright (C) 2018-2019
//     Verificarlo contributors
//     Universite de Versailles St-Quentin-en-Yvelines
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

#include <argp.h>
#include <err.h>
#include <errno.h>
#include <math.h>
#include <mpfr.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/time.h>
#include <unistd.h>

#include "../../common/float_const.h"
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
typedef enum {
  ieee,
  mca,
  pb,
  rr
} mcamode;

static const char *MCAMODE[] = {"ieee", "mca", "pb", "rr"};

/* define default environment variables and default parameters */
#define MCA_PRECISION_BINARY32_MIN 1
#define MCA_PRECISION_BINARY64_MIN 1
#define MCA_PRECISION_BINARY32_MAX MPFR_PREC_MAX
#define MCA_PRECISION_BINARY64_MAX MPFR_PREC_MAX
#define MCA_PRECISION_BINARY32_DEFAULT 24
#define MCA_PRECISION_BINARY64_DEFAULT 53
#define MCAMODE_DEFAULT mca

static mcamode MCALIB_OP_TYPE = MCAMODE_DEFAULT;
static int MCALIB_BINARY32_T = MCA_PRECISION_BINARY32_DEFAULT;
static int MCALIB_BINARY64_T = MCA_PRECISION_BINARY64_DEFAULT;

#define GET_MCALIB_T(X)                                                        \
  _Generic((X), float : MCALIB_BINARY32_T, double : MCALIB_BINARY64_T)
#define GET_PREC(X) _Generic((X), float : DOUBLE_PREC, double : QUAD_PREC)
#define MPFR_SET_FLT(X, RND)                                                   \
  _Generic((X), float                                                          \
           : mpfr_set_flt(mpfr_##X, X, RND), double                            \
           : mpfr_set_d(mpfr_##X, X, RND))
#define MPFR_GET_FLT(X, RND)                                                   \
  _Generic((X), float                                                          \
           : mpfr_get_flt(mpfr_##X, RND), double                               \
           : mpfr_get_d(mpfr_##X, RND))

#define MP_ADD &mpfr_add
#define MP_SUB &mpfr_sub
#define MP_MUL &mpfr_mul
#define MP_DIV &mpfr_div

typedef int (*mpfr_bin)(mpfr_t, mpfr_t, mpfr_t, mpfr_rnd_t);
typedef int (*mpfr_unr)(mpfr_t, mpfr_t, mpfr_rnd_t);

static float _mca_binary32_binary_op(float a, float b, mpfr_bin mpfr_op);
static float _mca_binary32_unary_op(float a, mpfr_unr mpfr_op);

static double _mca_binary64_binary_op(double a, double b, mpfr_bin mpfr_op);
static double _mca_binary64_unary_op(double a, mpfr_unr mpfr_op);

/******************** MCA CONTROL FUNCTIONS *******************
 * The following functions are used to set virtual precision and
 * MCA mode of operation.
 ***************************************************************/

static int _set_mca_mode(mcamode mode) {
  if (mode < ieee || mode > rr)
    return -1;

  MCALIB_OP_TYPE = mode;
  return 0;
}

static int _set_mca_precision_binary32(int precision) {
  if (precision < MCA_PRECISION_BINARY32_MIN) {
    errx(1, "interflop_mca_mpfr: invalid precision for binary32 type. Must be "
            "greater than 0");
  } else if (precision > MCA_PRECISION_BINARY32_MAX) {
    warnx("interflop_mca_mpfr: precision for binary32 type is too high, no "
          "noise will be added");
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
    warnx("interflop_mca_mpfr: precision for binary64 type is too high, no "
          "noise will be added");
  } else {
    MCALIB_BINARY64_T = precision;
  }
  return 0;
}

/******************** MCA RANDOM FUNCTIONS ********************
 * The following functions are used to calculate the random
 * perturbations used for MCA and apply these to MPFR format
 * operands
 ***************************************************************/

/* random generator internal state */
static tinymt64_t random_state;

static double _mca_rand(void) {
  /* Returns a random double in the (0,1) open interval */
  return tinymt64_generate_doubleOO(&random_state);
}

#define _MCA_INEXACT(X, rnd_mode)                                              \
  do {                                                                         \
    /* if we are in IEEE mode, we return a noise equal to 0 */                 \
    /* if a is NaN, Inf or 0, we don't disturb it */                           \
    if ((MCALIB_OP_TYPE == ieee) || (mpfr_regular_p(mpfr_##X) == 0)) { \
      break;                                                                   \
    }                                                                          \
    /* In RR, if the result is exact */                                        \
    /* in the current virtual precision,*/                                     \
    /* do not add  any noise  */                                               \
    mpfr_prec_t min_prec = mpfr_min_prec(mpfr_##X);                            \
    if (MCALIB_OP_TYPE == rr && min_prec <= GET_MCALIB_T(X)) {         \
      break;                                                                   \
    }                                                                          \
    /* get_exp reproduce frexp behavior,  */                                   \
    /* i.e. exp corresponding to a normalization */                            \
    /* in the interval [1/2 1[ */                                              \
    /* remove one to normalize in [1 2[ like ieee numbers */                   \
    mpfr_exp_t e_a = mpfr_get_exp(mpfr_##X) - 1;                               \
    mpfr_prec_t p_a = mpfr_get_prec(mpfr_##X);                                 \
    MPFR_DECL_INIT(mpfr_rand, p_a);                                            \
    e_a = e_a - (GET_MCALIB_T(X) - 1);                                         \
    double d_rand = (_mca_rand() - 0.5);                                       \
    mpfr_set_d(mpfr_rand, d_rand, rnd_mode);                                   \
    /* rand = rand * 2 ^ (e_a) */                                              \
    mpfr_mul_2si(mpfr_rand, mpfr_rand, e_a, rnd_mode);                         \
    mpfr_add(mpfr_##X, mpfr_##X, mpfr_rand, rnd_mode);                         \
  } while (0)

static void _set_mca_seed(bool choose_seed, uint64_t seed) {
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
 * are first converted to MPFR format, inbound and outbound perturbations
 * are applied using the _mca_inexact function, and the result converted
 * to the original format for return
 *******************************************************************/

#define _MCA_BINARY_OP(X, Y, OP)                                               \
  {                                                                            \
    mpfr_prec_t prec = GET_PREC(X);                                            \
    mpfr_rnd_t rnd = MPFR_RNDN;                                                \
    MPFR_DECL_INIT(mpfr_##X, prec);                                            \
    MPFR_DECL_INIT(mpfr_##Y, prec);                                            \
    MPFR_SET_FLT(X, rnd);                                                      \
    MPFR_SET_FLT(Y, rnd);                                                      \
    if (MCALIB_OP_TYPE != rr) {						\
      _MCA_INEXACT(X, rnd);                                                    \
      _MCA_INEXACT(Y, rnd);                                                    \
    }                                                                          \
    mpfr_op(mpfr_##X, mpfr_##X, mpfr_##Y, rnd);                                \
    if (MCALIB_OP_TYPE != pb) {                                        \
      _MCA_INEXACT(X, rnd);                                                    \
    }                                                                          \
    typeof(X) ret = MPFR_GET_FLT(X, rnd);                                      \
    return ret;                                                                \
  }

#define _MCA_UNARY_OP(X, OP)                                                   \
  {                                                                            \
    mpfr_prec_t prec = GET_PREC(X);                                            \
    mpfr_rnd_t rnd = MPFR_RNDN;                                                \
    MPFR_DECL_INIT(mpfr_##X, prec);                                            \
    MPFR_SET_FLT(X, rnd);                                                      \
    if (MCALIB_OP_TYPE != rr) {                                        \
      _MCA_INEXACT(X, rnd);                                                    \
    }                                                                          \
    mpfr_op(mpfr_a, mpfr_a, rnd);                                              \
    if (MCALIB_OP_TYPE != pb) {                                        \
      _MCA_INEXACT(X, rnd);                                                    \
    }                                                                          \
    typeof(X) ret = MPFR_GET_FLT(X, rnd);                                      \
    return ret;                                                                \
  }

static float _mca_binary32_binary_op(float a, float b, mpfr_bin mpfr_op) {
  _MCA_BINARY_OP(a, b, mpfr_op);
}

static float _mca_binary32_unary_op(float a, mpfr_unr mpfr_op) {
  _MCA_UNARY_OP(a, mpfr_op);
}

static double _mca_binary64_binary_op(double a, double b, mpfr_bin mpfr_op) {
  _MCA_BINARY_OP(a, b, mpfr_op);
}

static double _mca_binary64_unary_op(double a, mpfr_unr mpfr_op) {
  _MCA_UNARY_OP(a, mpfr_op);
}

/************************* FPHOOKS FUNCTIONS *************************
 * These functions correspond to those inserted into the source code
 * during source to source compilation and are replacement to floating
 * point operators
 **********************************************************************/

static void _interflop_add_float(float a, float b, float *c, void *context) {
  *c = _mca_binary32_binary_op(a, b, (mpfr_bin)MP_ADD);
}

static void _interflop_sub_float(float a, float b, float *c, void *context) {
  *c = _mca_binary32_binary_op(a, b, (mpfr_bin)MP_SUB);
}

static void _interflop_mul_float(float a, float b, float *c, void *context) {
  *c = _mca_binary32_binary_op(a, b, (mpfr_bin)MP_MUL);
}

static void _interflop_div_float(float a, float b, float *c, void *context) {
  *c = _mca_binary32_binary_op(a, b, (mpfr_bin)MP_DIV);
}

static void _interflop_add_double(double a, double b, double *c,
                                  void *context) {
  *c = _mca_binary64_binary_op(a, b, (mpfr_bin)MP_ADD);
}

static void _interflop_sub_double(double a, double b, double *c,
                                  void *context) {
  *c = _mca_binary64_binary_op(a, b, (mpfr_bin)MP_SUB);
}

static void _interflop_mul_double(double a, double b, double *c,
                                  void *context) {
  *c = _mca_binary64_binary_op(a, b, (mpfr_bin)MP_MUL);
}

static void _interflop_div_double(double a, double b, double *c,
                                  void *context) {
  *c = _mca_binary64_binary_op(a, b, (mpfr_bin)MP_DIV);
}

static struct argp_option options[] = {
    /* --debug, sets the variable debug = true */
    {"precision-binary32", KEY_PREC_B32, "PRECISION", 0,
     "select precision for binary32 (PRECISION >= 0)"},
    {"precision-binary64", KEY_PREC_B64, "PRECISION", 0,
     "select precision for binary64 (PRECISION >= 0)"},
    {"mode", 'm', "MODE", 0, "select MCA mode among {ieee, mca, pb, rr}"},
    {"seed", 's', "SEED", 0, "fix the random generator seed"},
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
      errx(1, "interflop_mca_mpfr: --mode invalid value provided, must be one "
              "of: {ieee, mca, pb, rr}.");
    }
    break;
  case KEY_SEED:
    errno = 0;
    ctx->choose_seed = true;
    ctx->seed = strtoull(arg, &endptr, 10);
    if (errno != 0) {
      errx(1, "interflop_mca_mpfr: --seed invalid value provided, must be an "
              "integer");
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

  warnx("interflop_mca_mpfr: loaded backend with precision-binary32 = %d, "
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
