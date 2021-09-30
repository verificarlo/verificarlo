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

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/syscall.h> // for getting the thread id
#include <sys/time.h>
#include <sys/types.h>
/* Modern solution for working with threads, since C11 */
/*  currently, support for threads.h is available from glibc2.28 onwards */
// #include <threads.h>
#include <unistd.h>

#include "tinymt64.h"

#include "options.h"

/* Data type used to hold information required by the RNG used for MCA */
// typedef struct mca_data {
//   bool *choose_seed;
//   unsigned long long int *seed;
//   bool *random_state_valid;
//   struct drand48_data *random_state;
//   pthread_mutex_t *global_tid_lock;
//   unsigned long long int *global_tid;
// } mca_data_t;

/* Generic set_seed function which is common for most of the backends */
void _set_seed_default(tinymt64_t *random_state, const bool choose_seed,
                       const uint64_t seed) {
  if (choose_seed) {
    tinymt64_init(random_state, seed);
  } else {
    const int key_length = 3;
    uint64_t init_key[key_length];
    struct timeval t1;
    gettimeofday(&t1, NULL);
    /* Hopefully the following seed is good enough for Montercarlo */
    init_key[0] = t1.tv_sec;
    init_key[1] = t1.tv_usec;
    init_key[2] = getpid();
    tinymt64_init_by_array(random_state, init_key, key_length);
  }
}

/* Simple set_seed function for the basic generators */
void _set_seed(struct drand48_data *random_state, const bool choose_seed,
               const unsigned long long int seed) {
  if (choose_seed) {
    // *random_state = seed;
    srand48_r((unsigned long int)seed, random_state);
  } else {
    struct timeval t1;
    unsigned long long int tmp_seed;
    unsigned short int tmp_seed_vect[3];

    gettimeofday(&t1, NULL);

    /* Hopefully the following seed is good enough for Montercarlo */
    // *random_state = t1.tv_sec ^ t1.tv_usec ^ syscall(__NR_gettid);
    tmp_seed = t1.tv_sec ^ t1.tv_usec ^ syscall(__NR_gettid);
    tmp_seed_vect[0] = (unsigned short int)(tmp_seed & 0x000000000000FFFF);
    tmp_seed_vect[1] =
        (unsigned short int)((tmp_seed & 0x00000000FFFF0000) >> 16);
    tmp_seed_vect[2] =
        (unsigned short int)((tmp_seed & 0x0000FFFF00000000) >> 32);
    seed48_r(tmp_seed_vect, random_state);
    /* Modern solution for working with threads, since C11 */
    // *random_state = t1.tv_sec ^ t1.tv_usec ^ thrd_current();
  }
}

/* Output a floating point number r (0.0 < r < 1.0) */
double generate_random_double(struct drand48_data *random_state) {
  double tmp_rand;

  drand48_r(random_state, &tmp_rand);
  while (tmp_rand == 0)
    drand48_r(random_state, &tmp_rand);

  return tmp_rand;
}

/* Initialize a data structure used to hold the information required */
/* by the RNG */
mca_data_t *get_mca_data_struct(bool *choose_seed, unsigned long long int *seed,
                                bool *random_state_valid,
                                struct drand48_data *random_state,
                                pthread_mutex_t *global_tid_lock,
                                unsigned long long int *global_tid) {
  mca_data_t *new_data = (mca_data_t *)malloc(sizeof(mca_data_t));

  new_data->choose_seed = choose_seed;
  new_data->seed = seed;
  new_data->random_state_valid = random_state_valid;
  new_data->random_state = random_state;
  new_data->global_tid_lock = global_tid_lock;
  new_data->global_tid = global_tid;

  return new_data;
}

/* Get a new identifier for the calling thread */
/* Generic threads can have inconsistent identifiers, assigned by the system, */
/* we therefore need to set an order between threads, for the case
/* when the seed is fixed, to insure some repeatability between executions */
unsigned long long int _get_new_tid(pthread_mutex_t *global_tid_lock,
                                    unsigned long long int *global_tid) {
  unsigned long long int tmp_tid = -1;

  pthread_mutex_lock(global_tid_lock);
  tmp_tid = *global_tid;
  *global_tid++;
  pthread_mutex_unlock(global_tid_lock);

  return tmp_tid;
}

/* Returns a random double in the (0,1) open interval */
double _mca_rand(mca_data_t *mca_data) {

  if (*(mca_data->random_state_valid) == false) {
    if (*(mca_data->choose_seed) == true) {
      _set_seed(mca_data->random_state, *(mca_data->choose_seed),
                *(mca_data->seed) ^ _get_new_tid(mca_data->global_tid_lock,
                                                 mca_data->global_tid));
    } else {
      _set_seed(mca_data->random_state, false, 0);
    }
    *(mca_data->random_state_valid) = true;
  }

  return generate_random_double(mca_data->random_state);
}

/* Returns a bool for determining whether an operation should skip */
/* perturbation. false -> perturb; true -> skip. */
/* e.g. for sparsity=0.1, all random values > 0.1 = true -> no MCA*/
bool _mca_skip_eval(const float sparsity, mca_data_t *mca_data) {

  if (sparsity >= 1.0f) {
    return false;
  }

  return (_mca_rand(mca_data) > sparsity);
}
