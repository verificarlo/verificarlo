/*****************************************************************************
 *                                                                           *
 *  This file is part of Verificarlo.                                        *
 *                                                                           *
 *  Copyright (c) 2015                                                       *
 *     Universite de Versailles St-Quentin-en-Yvelines                       *
 *     CMLA, Ecole Normale Superieure de Cachan                              *
 *  Copyright (c) 2018                                                       *
 *     Universite de Versailles St-Quentin-en-Yvelines                       *
 *  Copyright (c) 2018-2020                                                  *
 *     Verificarlo contributors                                              *
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


#include <argp.h>
#include <err.h>
#include <errno.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>

#include "../../common/float_const.h"
#include "../../common/float_struct.h"
#include "../../common/float_utils.h"
#include "../../common/interflop.h"
#include "../../common/logger.h"
#include "../../common/options.h"
#include "../../common/tinymt64.h"


#include "../../common/float_const.h"
#include "../../common/tinymt64.h"

typedef struct {
  bool choose_seed;
  uint64_t seed;
} t_context;

/* define default environment variables and default parameters */
#define TOLERANCE_DEFAULT 1
#define WARNING_DEFAULT 0

static int WARN = WARNING_DEFAULT;

static int TOLERANCE = TOLERANCE_DEFAULT;

// possible op values
#define MCA_ADD 1
#define MCA_SUB 2
#define MCA_MUL 3
#define MCA_DIV 4

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))
#define bit(A,i) ((A >> i) & 1)
#define bit_a_b(X,a,b) (((X << (sizeof(X)*8 - max(a,b) - 1)) >> (sizeof(X)*8 - max(a,b) - 1)) & ((X >> min(a,b) ) << min(a,b)))

static float _mca_sbin(float a, float b, int qop);
static double _mca_dbin(double a, double b, int qop);

/******************** MCA CONTROL FUNCTIONS *******************
 * The following functions are used to set virtual precision and
 * MCA mode of operation.
 ***************************************************************/

static int _set_mca_tolerance(int tolerance) {
  TOLERANCE = tolerance;
  return 0;
}

static int _set_warning() {
  WARN = 1;
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

#define exp(X) ({                                     \
  /* Convert X to an int */                           \
  u_int64_t _V_ = *(u_int64_t*)&X;                    \
  /* Get size of the exponent */                      \
  int _E_ = (6+3*(sizeof(X) >> 2)-1);                 \
  /* Get the size of the mantiss */                   \
  int _M_ = (sizeof(X)*8-_E_-1);                      \
  /* Compute the exponent */                          \
  (int)bit_a_b(_V_>>_M_,0,_E_-1) - ((1<<_E_-1)-2);})

#define inexact(X,S) (X + pow(2,exp(X)-S) * _mca_rand())

/* Set the mca seed */
static void _set_mca_seed(const bool choose_seed, const uint64_t seed) {
  _set_seed_default(&random_state, choose_seed, seed);
}

/****************** CANCELLATION DETECTION FUNCTIONS *************************
 *  Compute the difference between the max of both operands and the exposant
 * of the result to find the size of the cancellation
 ****************************************************************************/

#define detect(X,Y,Z) ((int)max(exp(X),exp(Y))-exp(Z))

/******************** MCA ARITHMETIC FUNCTIONS ********************
 * The following set of functions perform the MCA operation. Operands
 * are first converted to quad  format (GCC), inbound and outbound
 * perturbations are applied using the _mca_inexact function, and the
 * result converted to the original format for return
 *******************************************************************/

// perform_bin_op: applies the binary operator (op) to (a) and (b)
// and stores the result in (res)

#define mant(X) ((int)(sizeof(X)*8-(6+3*(sizeof(X) >> 2)-1)-1))

#define cancell(X,Y,Z) ({                                              \
  int cancellation = detect(X,Y,Z);                                    \
  if(cancellation >= TOLERANCE) {                                      \
    if(WARN) {                                                         \
      logger_info("cancellation of size %d detected\n", cancellation); \
    }                                                                  \
      int exp = (mant(Z)+1) - cancellation;                            \
      Z = inexact(Z,exp);                                              \
  }                                                                    \
  Z;})

/************************* FPHOOKS FUNCTIONS *************************
 * These functions correspond to those inserted into the source code
 * during source to source compilation and are replacement to floating
 * point operators
 **********************************************************************/

static void _interflop_add_float(float a, float b, float *c, void *context) {
  *c = a + b;
}

static void _interflop_sub_float(float a, float b, float *c, void *context) {
  *c = a - b;
  *c = cancell(a, b, *c);
}

static void _interflop_mul_float(float a, float b, float *c, void *context) {
  *c = a * b;
}

static void _interflop_div_float(float a, float b, float *c, void *context) {
  *c = a / b;
}

static void _interflop_add_double(double a, double b, double *c,
                                  void *context) {
  *c = a + b;
}

static void _interflop_sub_double(double a, double b, double *c,
                                  void *context) {
  *c = a - b;
  *c = cancell(a, b, *c);
}

static void _interflop_mul_double(double a, double b, double *c,
                                  void *context) {
  *c = a * b;
}

static void _interflop_div_double(double a, double b, double *c,
                                  void *context) {
  *c = a / b;
}

static struct argp_option options[] = {
    /* --debug, sets the variable debug = true */
    {"tolerance", 't', "TOLERANCE", 0, "select tolerance (tolerance >= 0)"},
    {"warning", 'w', "WARNING", 0, "active warning for cancellations"},
    {"seed", 's', "SEED", 0, "fix the random generator seed"},
    {0}};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  t_context *ctx = (t_context *)state->input;
  char *endptr;
  switch (key) {
  case 't':
    /* tolerance */
    errno = 0;
    int val = strtol(arg, &endptr, 10);
    if (errno != 0 || val < 0) {
      logger_error("--tolerance invalid value provided, must be a"
          "positive integer.");
    } else {
      _set_mca_tolerance(val);
    }
    break;
  case 'w':
      _set_warning();
    break;
  case 's':
    errno = 0;
    ctx->choose_seed = 1;
    ctx->seed = strtoull(arg, &endptr, 10);
    if (errno != 0) {
      logger_error("--seed invalid value provided, must be an integer");
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

  logger_init();

  _set_mca_tolerance(TOLERANCE_DEFAULT);

  t_context *ctx = malloc(sizeof(t_context));
  *context = ctx;
  init_context(ctx);


  /* parse backend arguments */
  argp_parse(&argp, argc, argv, 0, 0, ctx);

  logger_info("interflop_cancellation: loaded backend with tolerance = %d ",
        TOLERANCE);

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
