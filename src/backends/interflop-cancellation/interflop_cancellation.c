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
static tinymt64_t random_state;

/* Set the mca seed */
static void _set_mca_seed(const bool choose_seed, const uint64_t seed) {
  _set_seed_default(&random_state, choose_seed, seed);
}

static double _mca_rand(void) {
  /* Returns a random double in the (0,1) open interval */
  return tinymt64_generate_doubleOO(&random_state);
}

/* noise = rand * 2^(exp) */
static inline double _noise_binary64(const int exp) {
  const double d_rand = (_mca_rand() - 0.5);
  return _fast_pow2_binary64(exp) * d_rand;
}

/* cancell: detects the cancellation size; and checks if its larger than the
 * chosen tolerance. It reports a warning to the user and adds a MCA noise of
 * the magnitude of the cancelled bits. */
#define cancell(X, Y, Z)                                                       \
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
      *Z = *Z + _noise_binary64(e_n);                                          \
    }                                                                          \
  })

#define define_interflop_add_sub_vector(size, precision, ops, ope)             \
  static void _interflop_##ops##_##precision##_##size##x(precision##size *a,   \
                                                         precision##size *b,   \
                                                         precision##size *c,   \
                                                         void *context) {      \
    (*c) = (*a) ope (*b);                                                      \
    for (int i = 0; i < size; ++i) {                                           \
      precision _c = (*c)[i];                                                  \
      cancell((*a)[i], (*b)[i], &_c);                                          \
      (*c)[i] = _c;                                                            \
    }                                                                          \
  }

#define define_interflop_mul_div_vector(size, precision, ops, ope)             \
  static void _interflop_##ops##_##precision##_##size##x(precision##size *a,   \
                                                         precision##size *b,   \
                                                         precision##size *c,   \
                                                         void *context) {      \
    *c = *a ope *b;                                                            \
  }

/* Cancellations can only happen during additions and substractions */
static void _interflop_add_float(float a, float b, float *c, void *context) {
  *c = a + b;
  cancell(a, b, c);
}

static void _interflop_sub_float(float a, float b, float *c, void *context) {
  *c = a - b;
  cancell(a, b, c);
}

static void _interflop_add_double(double a, double b, double *c,
                                  void *context) {
  *c = a + b;
  cancell(a, b, c);
}

static void _interflop_sub_double(double a, double b, double *c,
                                  void *context) {
  *c = a - b;
  cancell(a, b, c);
}

/* Define float add and sub vector functions */
define_interflop_add_sub_vector(2, float, add, +);
define_interflop_add_sub_vector(2, float, sub, -);

define_interflop_add_sub_vector(4, float, add, +);
define_interflop_add_sub_vector(4, float, sub, -);

define_interflop_add_sub_vector(8, float, add, +);
define_interflop_add_sub_vector(8, float, sub, -);

define_interflop_add_sub_vector(16, float, add, +);
define_interflop_add_sub_vector(16, float, sub, -);

/* Define double add and sub vector functions */
define_interflop_add_sub_vector(2, double, add, +);
define_interflop_add_sub_vector(2, double, sub, -);

define_interflop_add_sub_vector(4, double, add, +);
define_interflop_add_sub_vector(4, double, sub, -);

define_interflop_add_sub_vector(8, double, add, +);
define_interflop_add_sub_vector(8, double, sub, -);

define_interflop_add_sub_vector(16, double, add, +);
define_interflop_add_sub_vector(16, double, sub, -);

static void _interflop_mul_float(float a, float b, float *c, void *context) {
  *c = a * b;
}

static void _interflop_div_float(float a, float b, float *c, void *context) {
  *c = a / b;
}

static void _interflop_mul_double(double a, double b, double *c,
                                  void *context) {
  *c = a * b;
}

static void _interflop_div_double(double a, double b, double *c,
                                  void *context) {
  *c = a / b;
}

/* Define float mul and div vector functions */
define_interflop_mul_div_vector(2, float, mul, *);
define_interflop_mul_div_vector(2, float, div, /);

define_interflop_mul_div_vector(4, float, mul, *);
define_interflop_mul_div_vector(4, float, div, /);

define_interflop_mul_div_vector(8, float, mul, *);
define_interflop_mul_div_vector(8, float, div, /);

define_interflop_mul_div_vector(16, float, mul, *);
define_interflop_mul_div_vector(16, float, div, /);

/* Define double mul and div vector functions */
define_interflop_mul_div_vector(2, double, mul, *);
define_interflop_mul_div_vector(2, double, div, /);

define_interflop_mul_div_vector(4, double, mul, *);
define_interflop_mul_div_vector(4, double, div, /);

define_interflop_mul_div_vector(8, double, mul, *);
define_interflop_mul_div_vector(8, double, div, /);

define_interflop_mul_div_vector(16, double, mul, *);
define_interflop_mul_div_vector(16, double, div, /);

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
      _interflop_add_float_2x,
      _interflop_sub_float_2x,
      _interflop_mul_float_2x,
      _interflop_div_float_2x,
      NULL,
      _interflop_add_float_4x,
      _interflop_sub_float_4x,
      _interflop_mul_float_4x,
      _interflop_div_float_4x,
      NULL,
      _interflop_add_float_8x,
      _interflop_sub_float_8x,
      _interflop_mul_float_8x,
      _interflop_div_float_8x,
      NULL,
      _interflop_add_float_16x,
      _interflop_sub_float_16x,
      _interflop_mul_float_16x,
      _interflop_div_float_16x,
      NULL,
      _interflop_add_double,
      _interflop_sub_double,
      _interflop_mul_double,
      _interflop_div_double,
      NULL,
      _interflop_add_double_2x,
      _interflop_sub_double_2x,
      _interflop_mul_double_2x,
      _interflop_div_double_2x,
      NULL,
      _interflop_add_double_4x,
      _interflop_sub_double_4x,
      _interflop_mul_double_4x,
      _interflop_div_double_4x,
      NULL,
      _interflop_add_double_8x,
      _interflop_sub_double_8x,
      _interflop_mul_double_8x,
      _interflop_div_double_8x,
      NULL,
      _interflop_add_double_16x,
      _interflop_sub_double_16x,
      _interflop_mul_double_16x,
      _interflop_div_double_16x,
      NULL,
      NULL,
      NULL,
      NULL};

  /* Initialize the seed */
  _set_mca_seed(ctx->choose_seed, ctx->seed);

  return interflop_backend_cancellation;
}
