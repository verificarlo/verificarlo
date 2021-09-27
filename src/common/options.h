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

#include <stdlib.h>

#include "float_const.h"
#include "logger.h"
#include "tinymt64.h"
#include <pthread.h>

/* Data type used to hold information required by the RNG used for MCA */
typedef struct mca_data {
  bool *choose_seed;
  unsigned long long int *seed;
  bool *random_state_valid;
  struct drand48_data *random_state;
  pthread_mutex_t *global_tid_lock;
  unsigned long long int *global_tid;
} mca_data_t;

/* Generic set_precision macro function which is common with most of the backend
 */
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

/* Generic set_seed function which is common for most of the backends */
void _set_seed_default(tinymt64_t *random_state, const bool choose_seed,
                       const uint64_t seed);

/* Simple set_seed function for the basic generators */
void _set_seed(struct drand48_data *random_state, const bool choose_seed,
               const unsigned long long int seed);

/* Output a floating point number r (0.0 < r < 1.0) */
double generate_random_double(struct drand48_data *random_state);

/* Initialize a data structure used to hold the information required */
/* by the RNG */
mca_data_t *get_mca_data_struct(bool *choose_seed, unsigned long long int *seed,
                                bool *random_state_valid,
                                struct drand48_data *random_state,
                                pthread_mutex_t *global_tid_lock,
                                unsigned long long int *global_tid);

double _mca_rand(mca_data_t *mca_data);

bool _mca_skip_eval(const float sparsity, mca_data_t *mca_data);

unsigned long long int _get_new_tid(pthread_mutex_t *global_tid_lock,
                                    unsigned long long int *global_tid);

#endif /* __OPTIONS_H__ */
