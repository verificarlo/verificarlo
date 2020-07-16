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
// Copyright (C) 2018-2020
//     Verificarlo contributors
//     Universite de Versailles St-Quentin-en-Yvelines
//
// Changelog:
//
// 2020-02-26 Use variables for options name instead of hardcoded one.
// Add DAZ/FTZ support.
//
// 2020-02-07 create separated virtual precisions for binary32
// and binary64. Uses a macro function for MCA_INEXACT for
// factorization purposes. Uses _Generic feature of c++11 standard
// for this purpose that implies a compiler that supports c++11 standard.
// Change return type from int to void for some functions and uses instead
// errx and warnx for handling errors.
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
#include "../../common/float_struct.h"
#include "../../common/float_utils.h"
#include "../../common/interflop.h"
#include "../../common/logger.h"
#include "../../common/options.h"
#include "../../common/tinymt64.h"

typedef enum {
  KEY_PREC_B32,
  KEY_PREC_B64,
  KEY_MODE = 'm',
  KEY_SEED = 's',
  KEY_DAZ = 'd',
  KEY_FTZ = 'f'
} key_args;

static const char key_prec_b32_str[] = "precision-binary32";
static const char key_prec_b64_str[] = "precision-binary64";
static const char key_mode_str[] = "mode";
static const char key_seed_str[] = "seed";
static const char key_daz_str[] = "daz";
static const char key_ftz_str[] = "ftz";

typedef struct {
  bool choose_seed;
  uint64_t seed;
  bool daz;
  bool ftz;
} t_context;

/* define the available MCA modes of operation */
typedef enum {
  mcamode_ieee,
  mcamode_mca,
  mcamode_pb,
  mcamode_rr,
  _mcamode_end_
} mcamode;

static const char *MCA_MODE_STR[] = {"ieee", "mca", "pb", "rr"};

/* define default parameters */
#define MCA_PRECISION_BINARY32_MIN 1
#define MCA_PRECISION_BINARY64_MIN 1
#define MCA_PRECISION_BINARY32_MAX 4096
#define MCA_PRECISION_BINARY64_MAX 4096
#define MCA_PRECISION_BINARY32_DEFAULT 24
#define MCA_PRECISION_BINARY64_DEFAULT 53
#define MCA_MODE_DEFAULT mcamode_mca

static mcamode MCALIB_MODE_TYPE = MCA_MODE_DEFAULT;
static int MCALIB_BINARY32_T = MCA_PRECISION_BINARY32_DEFAULT;
static int MCALIB_BINARY64_T = MCA_PRECISION_BINARY64_DEFAULT;

/* Generic macro that returns the virtual_precision corresponding to the type of
 * X */
#define GET_MCALIB_T(X)                                                        \
  _Generic((X), float : MCALIB_BINARY32_T, double : MCALIB_BINARY64_T)

/* Generic macro that returns the MPFR precision used for computing depending on
 * the type of X */
#define GET_MPFR_PREC(X) _Generic((X), float : DOUBLE_PREC, double : QUAD_PREC)

/* Generic macro that returns the MPFR setter depending on the type of X  */
#define MPFR_SET_FLT(X, RND)                                                   \
  _Generic((X), float                                                          \
           : mpfr_set_flt(mpfr_##X, X, RND), double                            \
           : mpfr_set_d(mpfr_##X, X, RND))

/* Generic macro that returns the MPFR getter depending on the type of X  */
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

static float _mca_binary32_binary_op(float a, float b, mpfr_bin mpfr_op,
                                     void *context);
static float _mca_binary32_unary_op(float a, mpfr_unr mpfr_op, void *context);

static double _mca_binary64_binary_op(double a, double b, mpfr_bin mpfr_op,
                                      void *context);
static double _mca_binary64_unary_op(double a, mpfr_unr mpfr_op, void *context);

/******************** MCA CONTROL FUNCTIONS *******************
 * The following functions are used to set virtual precision and
 * MCA mode of operation.
 ***************************************************************/

/* Set the mca mode */
static void _set_mca_mode(mcamode mode) {
  if (mode >= _mcamode_end_) {
    logger_error("--%s invalid value provided, must be one of: "
                 "{ieee, mca, pb, rr}.",
                 key_mode_str);
  }
  MCALIB_MODE_TYPE = mode;
}

/* Set the virtual precision for binary32 */
static void _set_mca_precision_binary32(int precision) {
  _set_precision(MCA, precision, &MCALIB_BINARY32_T, (float)0);
}

/* Set the virtual precision for binary64 */
static void _set_mca_precision_binary64(int precision) {
  _set_precision(MCA, precision, &MCALIB_BINARY64_T, (double)0);
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

/* Set the mca seed */
static void _set_mca_seed(bool choose_seed, uint64_t seed) {
  _set_seed_default(&random_state, choose_seed, seed);
}

/* Macro function that add mca noise to X */
#define _MCA_INEXACT(X, rnd_mode)                                              \
  do {                                                                         \
    /* if we are in IEEE mode, we return a noise equal to 0 */                 \
    /* if a is NaN, Inf or 0, we don't disturb it */                           \
    if ((MCALIB_MODE_TYPE == mcamode_ieee) ||                                  \
        (mpfr_regular_p(mpfr_##X) == 0)) {                                     \
      break;                                                                   \
    }                                                                          \
    /* In RR, if the result is exact */                                        \
    /* in the current virtual precision,*/                                     \
    /* do not add  any noise  */                                               \
    mpfr_prec_t min_prec = mpfr_min_prec(mpfr_##X);                            \
    if (MCALIB_MODE_TYPE == mcamode_rr && min_prec <= GET_MCALIB_T(X)) {       \
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

/******************** MCA ARITHMETIC FUNCTIONS ********************
 * The following set of functions perform the MCA operation. Operands
 * are first converted to MPFR format, inbound and outbound perturbations
 * are applied using the _mca_inexact function, and the result converted
 * to the original format for return
 *******************************************************************/

/* Generic macro function that returns mca(X OP Y) */
#define _MCA_BINARY_OP(X, Y, OP, CTX)                                          \
  {                                                                            \
    mpfr_prec_t prec = GET_MPFR_PREC(X);                                       \
    mpfr_rnd_t rnd = MPFR_RNDN;                                                \
    if (((t_context *)CTX)->daz) {                                             \
      X = DAZ(X);                                                              \
      Y = DAZ(Y);                                                              \
    }                                                                          \
    MPFR_DECL_INIT(mpfr_##X, prec);                                            \
    MPFR_DECL_INIT(mpfr_##Y, prec);                                            \
    MPFR_SET_FLT(X, rnd);                                                      \
    MPFR_SET_FLT(Y, rnd);                                                      \
    if (MCALIB_MODE_TYPE != mcamode_rr) {                                      \
      _MCA_INEXACT(X, rnd);                                                    \
      _MCA_INEXACT(Y, rnd);                                                    \
    }                                                                          \
    mpfr_op(mpfr_##X, mpfr_##X, mpfr_##Y, rnd);                                \
    if (MCALIB_MODE_TYPE != mcamode_pb) {                                      \
      _MCA_INEXACT(X, rnd);                                                    \
    }                                                                          \
    typeof(X) ret = MPFR_GET_FLT(X, rnd);                                      \
    if (((t_context *)CTX)->ftz) {                                             \
      ret = FTZ(ret);                                                          \
    }                                                                          \
    return ret;                                                                \
  }

/* Generic macro function that returns mca(OP X) */
#define _MCA_UNARY_OP(X, OP, CTX)                                              \
  {                                                                            \
    mpfr_prec_t prec = GET_MPFR_PREC(X);                                       \
    mpfr_rnd_t rnd = MPFR_RNDN;                                                \
    if (((t_context *)CTX)->daz) {                                             \
      X = DAZ(X);                                                              \
    }                                                                          \
    MPFR_DECL_INIT(mpfr_##X, prec);                                            \
    MPFR_SET_FLT(X, rnd);                                                      \
    if (MCALIB_MODE_TYPE != mcamode_rr) {                                      \
      _MCA_INEXACT(X, rnd);                                                    \
    }                                                                          \
    mpfr_op(mpfr_a, mpfr_a, rnd);                                              \
    if (MCALIB_MODE_TYPE != mcamode_pb) {                                      \
      _MCA_INEXACT(X, rnd);                                                    \
    }                                                                          \
    typeof(X) ret = MPFR_GET_FLT(X, rnd);                                      \
    if (((t_context *)CTX)->ftz) {                                             \
      ret = FTZ(ret);                                                          \
    }                                                                          \
    return ret;                                                                \
  }

/* Performs mca(a mpfr_op b) where a and b are binary32 values */
/* Intermediate computations are performed with precision DOUBLE_PREC */
static float _mca_binary32_binary_op(float a, float b, mpfr_bin mpfr_op,
                                     void *context) {
  _MCA_BINARY_OP(a, b, mpfr_op, context);
}

/* Performs mca(mpfr_op a) where a is a binary32 value */
/* Intermediate computations are performed with precision DOUBLE_PREC */
static float _mca_binary32_unary_op(float a, mpfr_unr mpfr_op, void *context) {
  _MCA_UNARY_OP(a, mpfr_op, context);
}

/* Performs mca(a mpfr_op b) where a and b are binary32 values */
/* Intermediate computations are performed with precision QUAD_PREC */
static double _mca_binary64_binary_op(double a, double b, mpfr_bin mpfr_op,
                                      void *context) {
  _MCA_BINARY_OP(a, b, mpfr_op, context);
}

/* Performs mca(mpfr_op a) where a is a binary32 value */
/* Intermediate computations are performed with precision QUAD_PREC */
static double _mca_binary64_unary_op(double a, mpfr_unr mpfr_op,
                                     void *context) {
  _MCA_UNARY_OP(a, mpfr_op, context);
}

/************************* FPHOOKS FUNCTIONS *************************
 * These functions correspond to those inserted into the source code
 * during source to source compilation and are replacement to floating
 * point operators
 **********************************************************************/

static void _interflop_add_float(float a, float b, float *c, void *context) {
  *c = _mca_binary32_binary_op(a, b, (mpfr_bin)MP_ADD, context);
}

static void _interflop_sub_float(float a, float b, float *c, void *context) {
  *c = _mca_binary32_binary_op(a, b, (mpfr_bin)MP_SUB, context);
}

static void _interflop_mul_float(float a, float b, float *c, void *context) {
  *c = _mca_binary32_binary_op(a, b, (mpfr_bin)MP_MUL, context);
}

static void _interflop_div_float(float a, float b, float *c, void *context) {
  *c = _mca_binary32_binary_op(a, b, (mpfr_bin)MP_DIV, context);
}

static void _interflop_add_double(double a, double b, double *c,
                                  void *context) {
  *c = _mca_binary64_binary_op(a, b, (mpfr_bin)MP_ADD, context);
}

static void _interflop_sub_double(double a, double b, double *c,
                                  void *context) {
  *c = _mca_binary64_binary_op(a, b, (mpfr_bin)MP_SUB, context);
}

static void _interflop_mul_double(double a, double b, double *c,
                                  void *context) {
  *c = _mca_binary64_binary_op(a, b, (mpfr_bin)MP_MUL, context);
}

static void _interflop_div_double(double a, double b, double *c,
                                  void *context) {
  *c = _mca_binary64_binary_op(a, b, (mpfr_bin)MP_DIV, context);
}

static struct argp_option options[] = {
    {key_prec_b32_str, KEY_PREC_B32, "PRECISION", 0,
     "select precision for binary32 (PRECISION > 0)", 0},
    {key_prec_b64_str, KEY_PREC_B64, "PRECISION", 0,
     "select precision for binary64 (PRECISION > 0)", 0},
    {key_mode_str, KEY_MODE, "MODE", 0,
     "select MCA mode among {ieee, mca, pb, rr}", 0},
    {key_seed_str, KEY_SEED, "SEED", 0, "fix the random generator seed", 0},
    {key_daz_str, KEY_DAZ, 0, 0,
     "denormals-are-zero: sets denormals inputs to zero", 0},
    {key_ftz_str, KEY_FTZ, 0, 0, "flush-to-zero: sets denormal output to zero",
     0},
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
      logger_error("--%s invalid value provided, must be a positive integer",
                   key_prec_b32_str);
    } else {
      _set_mca_precision_binary32(val);
    }
    break;
  case KEY_PREC_B64:
    /* precision for binary64 */
    errno = 0;
    val = strtol(arg, &endptr, 10);
    if (errno != 0 || val <= 0) {
      logger_error("--%s invalid value provided, must be a positive integer",
                   key_prec_b64_str);
    } else {
      _set_mca_precision_binary64(val);
    }
    break;
  case KEY_MODE:
    /* mode */
    if (strcasecmp(MCA_MODE_STR[mcamode_ieee], arg) == 0) {
      _set_mca_mode(mcamode_ieee);
    } else if (strcasecmp(MCA_MODE_STR[mcamode_mca], arg) == 0) {
      _set_mca_mode(mcamode_mca);
    } else if (strcasecmp(MCA_MODE_STR[mcamode_pb], arg) == 0) {
      _set_mca_mode(mcamode_pb);
    } else if (strcasecmp(MCA_MODE_STR[mcamode_rr], arg) == 0) {
      _set_mca_mode(mcamode_rr);
    } else {
      logger_error("--%s invalid value provided, must be one of: "
                   "{ieee, mca, pb, rr}.",
                   key_mode_str);
    }
    break;
  case KEY_SEED:
    /* set seed */
    errno = 0;
    ctx->choose_seed = true;
    ctx->seed = strtoull(arg, &endptr, 10);
    if (errno != 0) {
      logger_error("--%s invalid value provided, must be an integer",
                   key_seed_str);
    }
    break;
  case KEY_DAZ:
    /* denormal-are-zero */
    ctx->daz = true;
    break;
  case KEY_FTZ:
    /* flush-to-zero */
    ctx->ftz = true;
    break;
  default:
    return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

static struct argp argp = {options, parse_opt, "", "", NULL, NULL, NULL};

static void init_context(t_context *ctx) {
  ctx->choose_seed = false;
  ctx->seed = 0ULL;
}

/* Displays arguments when the backend is loaded */
static void print_information_header(void *context) {
  t_context *ctx = (t_context *)context;

  logger_info("load backend with "
              "%s = %d, "
              "%s = %d, "
              "%s = %s, "
              "%s = %s and "
              "%s = %s"
              "\n",
              key_prec_b32_str, MCALIB_BINARY32_T, key_prec_b64_str,
              MCALIB_BINARY64_T, key_mode_str, MCA_MODE_STR[MCALIB_MODE_TYPE],
              key_daz_str, ctx->daz ? "true" : "false", key_ftz_str,
              ctx->ftz ? "true" : "false");
}

struct interflop_backend_interface_t interflop_init(int argc, char **argv,
                                                    void **context) {

  /* Initialize the logger */
  logger_init();

  _set_mca_precision_binary32(MCA_PRECISION_BINARY32_DEFAULT);
  _set_mca_precision_binary64(MCA_PRECISION_BINARY64_DEFAULT);
  _set_mca_mode(MCA_MODE_DEFAULT);

  t_context *ctx = malloc(sizeof(t_context));
  *context = ctx;
  init_context(ctx);

  /* parse backend arguments */
  argp_parse(&argp, argc, argv, 0, 0, ctx);

  print_information_header(ctx);

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
      NULL,
      NULL,
      NULL,
      NULL};

  /* Initialize the seed */
  _set_mca_seed(ctx->choose_seed, ctx->seed);

  return interflop_backend_mca;
}
