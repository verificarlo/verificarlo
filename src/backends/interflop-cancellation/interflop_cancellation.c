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

#define max(a, b) ((a) > (b) ? (a) : (b))

static void _set_tolerance(int tolerance) { TOLERANCE = tolerance; }

static void _set_warning(bool warning) { WARN = warning; }

/* random generator internal state */
// static tinymt64_t random_state;

/* random number generator internal state */
static __thread struct drand48_data random_state;
/* random number generator initialization flag */
static __thread bool random_state_valid = false;

/* global thread id access lock */
static pthread_mutex_t global_tid_lock = PTHREAD_MUTEX_INITIALIZER;
/* global thread identifier */
static unsigned long long int global_tid = 0;

/* helper data structure to centralize the data used for random number
 * generation */
static __thread rng_state_t *rng_state;

/* Set the mca seed */
static void _set_mca_seed(const bool choose_seed,
                          const unsigned long long int seed) {
  _set_seed(&random_state, choose_seed, seed);
}

/* noise = rand * 2^(exp) */
static inline double _noise_binary64(const int exp, rng_state_t *rng_state) {
  const double d_rand = (_get_rand(rng_state) - 0.5);
  return _fast_pow2_binary64(exp) * d_rand;
}

/* Macro function that initializes the structure used for managing the RNG */
#define _INIT_RNG_STATE(CTX, RNG_STATE, RND_STATE, RND_STATE_VALID,            \
                        GLB_TID_LOCK, GLB_TID)                                 \
  {                                                                            \
    t_context *TMP_CTX = (t_context *)CTX;                                     \
    if (RNG_STATE == NULL) {                                                   \
      RNG_STATE = get_rng_state_struct(                                        \
          &(TMP_CTX->choose_seed), (unsigned long long *)(&(TMP_CTX->seed)),   \
          &RND_STATE_VALID, &RND_STATE, &GLB_TID_LOCK, &GLB_TID);              \
    }                                                                          \
  }

/* cancell: detects the cancellation size; and checks if its larger than the
 * chosen tolerance. It reports a warning to the user and adds a MCA noise of
 * the magnitude of the cancelled bits. */
#define cancell(X, Y, Z, CTX, RNG_STATE)                                       \
  ({                                                                           \
    const int32_t e_z = GET_EXP_FLT(*Z);                                       \
    /* computes the difference between the max of both operands and the        \
     * exponent of the result to find the size of the cancellation */          \
    int cancellation = max(GET_EXP_FLT(X), GET_EXP_FLT(Y)) - e_z;              \
    if (cancellation >= TOLERANCE) {                                           \
      if (WARN) {                                                              \
        logger_info("cancellation of size %d detected\n", cancellation);       \
      }                                                                        \
      /* Add an MCA noise of the magnitude of cancelled bits.                  \
       * This particular version in the case of cancellations does not use     \
       * extended quad types */                                                \
      const int32_t e_n = e_z - (cancellation - 1);                            \
      _INIT_RNG_STATE(CTX, RNG_STATE, random_state, random_state_valid,        \
                      global_tid_lock, global_tid);                            \
      *Z = *Z + _noise_binary64(e_n, RNG_STATE);                               \
    }                                                                          \
  })

/* Cancellations can only happen during additions and substractions */
static void _interflop_add_float(float a, float b, float *c, void *context) {
  *c = a + b;
  cancell(a, b, c, context, rng_state);
}

static void _interflop_sub_float(float a, float b, float *c, void *context) {
  *c = a - b;
  cancell(a, b, c, context, rng_state);
}

static void _interflop_add_double(double a, double b, double *c,
                                  void *context) {
  *c = a + b;
  cancell(a, b, c, context, rng_state);
}

static void _interflop_sub_double(double a, double b, double *c,
                                  void *context) {
  *c = a - b;
  cancell(a, b, c, context, rng_state);
}

static void _interflop_mul_float(float a, float b, float *c,
                                 __attribute__((unused)) void *context) {
  *c = a * b;
}

static void _interflop_div_float(float a, float b, float *c,
                                 __attribute__((unused)) void *context) {
  *c = a / b;
}

static void _interflop_mul_double(double a, double b, double *c,
                                  __attribute__((unused)) void *context) {
  *c = a * b;
}

static void _interflop_div_double(double a, double b, double *c,
                                  __attribute__((unused)) void *context) {
  *c = a / b;
}

static struct argp_option options[] = {
    {"tolerance", 't', "TOLERANCE", 0, "Select tolerance (TOLERANCE >= 0)", 0},
    {"warning", 'w', "WARNING", 0, "Enable warning for cancellations", 0},
    {"seed", 's', "SEED", 0, "Fix the random generator seed", 0},
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
      _set_tolerance(val);
    }
    break;
  case 'w':
    _set_warning(true);
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

static struct argp argp = {options, parse_opt, "", "", NULL, NULL, NULL};

static void init_context(t_context *ctx) {
  ctx->choose_seed = 0;
  ctx->seed = 0ULL;
}

void _interflop_finalize(__attribute__((unused)) void *context) {
  if (rng_state)
    free(rng_state);
}

struct interflop_backend_interface_t interflop_init(int argc, char **argv,
                                                    void **context) {

  logger_init();

  _set_tolerance(TOLERANCE_DEFAULT);

  t_context *ctx = malloc(sizeof(t_context));
  *context = ctx;
  init_context(ctx);

  /* parse backend arguments */
  argp_parse(&argp, argc, argv, 0, 0, ctx);

  logger_info("interflop_cancellation: loaded backend with tolerance = %d\n",
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
      NULL,
      NULL,
      NULL,
      NULL};

  /* The seed for the RNG is initialized upon the first request for a random
     number */

  rng_state = get_rng_state_struct(
      &(ctx->choose_seed), (unsigned long long int *)(&(ctx->seed)),
      &random_state_valid, &random_state, &global_tid_lock, &global_tid);

  return interflop_backend_cancellation;
}
