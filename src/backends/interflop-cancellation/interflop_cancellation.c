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
#include <string.h>

#include "../../common/float_const.h"
#include "../../common/interflop.h"
#include "../../common/tinymt64.h"

#include "quadmath-imp.h"

typedef struct {
  int choose_seed;
  uint64_t seed;
} t_context;

/* define default environment variables and default parameters */
#define MCA_PRECISION_DEFAULT 1

static int MCALIB_T = MCA_PRECISION_DEFAULT;

// possible op values
#define MCA_ADD 1
#define MCA_SUB 2
#define MCA_MUL 3
#define MCA_DIV 4

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))
#define bit(A,i) ((A >> i) & 1)
#define bit_a_b(X,a,b) (((X << (sizeof(X)*8 - max(a,b) - 1)) >> (sizeof(X)*8 - max(a,b) - 1)) & ((X >> min(a,b) ) << min(a,b)))
#define expd(X) ((X << 1) >> 24)
#define expq(X) ((X << 1) >> 53)

static float _mca_sbin(float a, float b, int qop);

static double _mca_dbin(double a, double b, int qop);

/******************** MCA CONTROL FUNCTIONS *******************
 * The following functions are used to set virtual precision and
 * MCA mode of operation.
 ***************************************************************/

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

static int _mca_inexactq(double* qa, int size) {
  int ex = 0;
  frexp((*qa), &ex);
  (*qa) = (*qa) + pow(2,ex-size) * (_mca_rand()-0.5);
  return 1;
}

static int _mca_inexactd(float* da, int size) {
  int ex = 0;
  frexp((*da), &ex);
  (*da) = (*da) + pow(2,ex-size) * (_mca_rand()-0.5);
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

/****************** CANCELLATION DETECTION FUNCTIONS *************************
 *
 * CANCELLATION DETECTION DOCUMENTAION
 *
 ****************************************************************************/

// return the number of common bit between two double
int cancell_double(double d1, double d2)
{
  if(d1 == d2)  return 0;

  int ea, eb, er;
  frexp(d1, &ea);
  frexp(d2, &eb);
  frexp(d1-d2, &er);

  return max(ea,eb) - er;
}

// return the number of common bit between two float
int cancell_float(float f1, float f2)
{  
  if(f1 == f2)  return 0;

  int ea, eb, er;
  frexpf(f1, &ea);
  frexpf(f2, &eb);
  frexpf(f1-f2, &er);

  return max(ea,eb) - er; 
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
  float res = 0;

  perform_bin_op(dop, res, a, b);

  int cancellation = cancell_float(a, b);

  //printf("%a - %a cancellation =  %d\n", a, b, cancellation);

  if (cancellation >= MCALIB_T) {
    _mca_inexactd(&res, 24 - cancellation);
  }

  return res;
}

static inline double _mca_dbin(double a, double b, const int qop) {
  double res = 0;

  perform_bin_op(qop, res, a, b);

  int cancellation = cancell_double(a, b);

  //printf("%la - %la cancellation =  %d\n", a, b, cancellation);

  if (cancellation >= MCALIB_T) {
    _mca_inexactq(&res, 53 - cancellation);
  }

  return res;
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
    if (errno != 0 || val < 0) {
      errx(1, "interflop_cancellation: --precision invalid value provided, must be a "
              "positive integer.");
    } else {
      _set_mca_precision(val);
    }
    break;
  case 's':
    errno = 0;
    ctx->choose_seed = 1;
    ctx->seed = strtoull(arg, &endptr, 10);
    if (errno != 0) {
      errx(1,
           "interflop_cancelletion: --seed invalid value provided, must be an integer");
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

  t_context *ctx = malloc(sizeof(t_context));
  *context = ctx;
  init_context(ctx);

  /* parse backend arguments */
  argp_parse(&argp, argc, argv, 0, 0, ctx);

  warnx("interflop_cancellation: loaded backend with precision = %d ",
        MCALIB_T);

  struct interflop_backend_interface_t interflop_backend_cancellation = {
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

  return interflop_backend_cancellation;
}
