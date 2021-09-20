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

#include <stdio.h>

#include <stdbool.h>
#include <stdint.h>
#include <sys/syscall.h> // for getting the thread id
#include <sys/time.h>
#include <sys/types.h>
/* Modern solution for working with threads, since C11 */
/*  currently, support for threads.h is available from glibc2.28 onwards */
// #include <threads.h>
#include <unistd.h>

#include <stdlib.h>

#include "tinymt64.h"

/* Data type used to hold information required by the RNG used for MCA */
typedef struct mca_data {
  bool *choose_seed;
  unsigned long long int *seed;
  bool *random_state_valid;
  unsigned long long int *random_state;
} mca_data_t;

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
void _set_seed(unsigned long long int *random_state, const bool choose_seed,
               const unsigned long long int seed) {
  if (choose_seed) {
    *random_state = seed;
  } else {
    struct timeval t1;

    gettimeofday(&t1, NULL);

    /* Hopefully the following seed is good enough for Montercarlo */
    *random_state = t1.tv_sec ^ t1.tv_usec ^ syscall(__NR_gettid);
    /* Modern solution for working with threads, since C11 */
    // *random_state = t1.tv_sec ^ t1.tv_usec ^ thrd_current();
  }
}

/* Output a floating point number r (0.0 < r < 1.0) */
double generate_random_double(unsigned long long int *random_state) {
  unsigned int tmp_state = (unsigned int)(*random_state);
  int tmp = rand_r(&tmp_state);
  *random_state = tmp_state;

  if (tmp == 0)
    tmp++;
  else if (tmp == RAND_MAX)
    tmp--;

  return ((double)1.0 * tmp) / RAND_MAX;
}

/* Initialize a data structure used to hold the information required */
/* by the RNG */
mca_data_t *get_mca_data_struct(bool *choose_seed, unsigned long long int *seed,
                                bool *random_state_valid,
                                unsigned long long int *random_state) {
  mca_data_t *new_data = (mca_data_t *)malloc(sizeof(mca_data_t));

  new_data->choose_seed = choose_seed;
  new_data->seed = seed;
  new_data->random_state_valid = random_state_valid;
  new_data->random_state = random_state;

  return new_data;
}
