/*****************************************************************************
 *                                                                           *
 *  This file is part of Verificarlo.                                        *
 *                                                                           *
 *  Copyright (c) 2015                                                       *
 *     Universite de Versailles St-Quentin-en-Yvelines                       *
 *     CMLA, Ecole Normale Superieure de Cachan                              *
 *  Copyright (c) 2018-2020                                                  *
 *     Verificarlo contributors                                              *
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

#ifndef __OPTIONS_H__
#define __OPTIONS_H__

#include <pthread.h>

#include "float_const.h"
#include "logger.h"
#include "tinymt64.h"

/* Data type used to hold information required by the RNG */
typedef struct rng_state {
  bool choose_seed;
  unsigned long long int seed;
  bool random_state_valid;
  struct drand48_data random_state;
  pthread_mutex_t *global_tid_lock;
  unsigned long long int *global_tid;
} rng_state_t;

/* A macro to simplify the generation of calls to the interflop hook functions
 */
/* TYPE      is the data type of the arguments */
/* OP_NAME   is the name of the operation that is being intercepted (i.e. add,
 * sub, mul, div) */
/* OP_TYPE   is the internal type used to represent the operations, specific to
 * each backend */
/* FUNC_NAME is the name of function used to emulated the intercepted call */
#define _INTERFLOP_OP_CALL(TYPE, OP_NAME, OP_TYPE, FUNC_NAME)                  \
  static void _interflop_##OP_NAME##_##TYPE(TYPE a, TYPE b, TYPE *c,           \
                                            void *context) {                   \
    *c = FUNC_NAME(a, b, OP_TYPE, context);                                    \
  }

/* Generic set_precision macro function which is common within most backends */
/* BACKEND   is the name of the backend */
/* PRECISION is the virtual precision to use */
/* T         is a pointer to the virtual precision variable */
/* X         is used to determine the floating-point type */
#define _set_precision(BACKEND, PRECISION, T, X)                               \
  {                                                                            \
    const int PRECISION_MIN =                                                  \
        _Generic(X, float                                                      \
                 : BACKEND##_PRECISION_BINARY32_MIN, double                    \
                 : BACKEND##_PRECISION_BINARY64_MIN);                          \
    const int PRECISION_MAX =                                                  \
        _Generic(X, float                                                      \
                 : BACKEND##_PRECISION_BINARY32_MAX, double                    \
                 : BACKEND##_PRECISION_BINARY64_MAX);                          \
    char type[] = _Generic(X, float : "binary32", double : "binary64");        \
    if (PRECISION < PRECISION_MIN) {                                           \
      logger_error("invalid precision for %s type. Must be greater than %d",   \
                   type, PRECISION_MIN);                                       \
    } else if (PRECISION > PRECISION_MAX) {                                    \
      logger_warning("precision (%d) for %s type is too high (%d), no noise "  \
                     "will be added",                                          \
                     PRECISION, type, PRECISION_MAX);                          \
    }                                                                          \
    *T = PRECISION;                                                            \
  }

/* Macro function that initializes the structure used for managing the RNG */
/* assumes the backend's context has a seed and choose_seed field */
/* CTX          is a pointer to the backend's context */
/* RNG_STATE    is the data structure that holds all the RNG-related data */
/* GLB_TID_LOCK is a pointer to the mutex for access to the unique TID */
/* GLB_TID      is a pointer to the unique TID */
#define _INIT_RNG_STATE(CTX, RNG_STATE, GLB_TID_LOCK, GLB_TID)                 \
  {                                                                            \
    t_context *TMP_CTX = (t_context *)CTX;                                     \
    if (RNG_STATE.global_tid == NULL) {                                        \
      init_rng_state_struct(&RNG_STATE, TMP_CTX->choose_seed,                   \
                           (unsigned long long)(TMP_CTX->seed), false,         \
                           &GLB_TID_LOCK, &GLB_TID);                           \
    }                                                                          \
  }

/* DEPRECATED */
/* Generic set_seed function which is common for most of the backends */
/* @param random state pointer to the internal state of the RNG */
/* @param choose_seed whether to set the seed to a user-provided value */
/* @param seed the user-provided seed for the RNG */
void _set_seed_default(tinymt64_t *random_state, const bool choose_seed,
                       const uint64_t seed);

/* Initialize the data structure used to hold the information required */
/* by the RNG */
/* @param rng_state pointer to the structure holding all the RNG-related data */
/* @param choose_seed whether to set the seed to a user-provided value */
/* @param seed the user-provided seed for the RNG */
/* @param random_state_valid whether the RNG internal state has been initialized
 */
/* @param global_tid_lock pointer to the mutex controling the access to the
 * Unique TID */
/* @param global_tid pointer to the unique TID */
void init_rng_state_struct(rng_state_t *rng_state, bool choose_seed,
                          unsigned long long int seed, bool random_state_valid,
                          pthread_mutex_t *global_tid_lock,
                          unsigned long long int *global_tid);

/* Get a new identifier for the calling thread */
/* Generic threads can have inconsistent identifiers, assigned by the system, */
/* we therefore need to set an order between threads, for the case
/* when the seed is fixed, to insure some repeatability between executions */
/* @param global_tid_lock pointer to the mutex controling the access to the
 * Unique TID */
/* @param global_tid pointer to the unique TID */
/* @return a new unique identifier for each calling thread*/
unsigned long long int _get_new_tid(pthread_mutex_t *global_tid_lock,
                                    unsigned long long int *global_tid);

/* Returns a random double in the (0,1) open interval */
/* Manages the internal state of the RNG, if necessary */
/* @param rng_state pointer to the structure holding all the RNG-related data */
/* @return a floating point number r (0.0 < r < 1.0) */
double _get_rand(rng_state_t *rng_state);

/* Returns a bool for determining whether an operation should skip */
/* perturbation. false -> perturb; true -> skip. */
/* e.g. for sparsity=0.1, all random values > 0.1 = true -> no MCA*/
/* @param sparsity sparsity */
/* @param rng_state pointer to the structure holding all the RNG-related data */
/* @return false -> perturb; true -> skip */
bool _mca_skip_eval(const float sparsity, rng_state_t *rng_state);

#endif /* __OPTIONS_H__ */
