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
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <stdlib.h>

#include "tinymt64.h"

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
void _set_seed_simple(unsigned int *random_state, const bool choose_seed,
                      const unsigned int seed) {
  if (choose_seed) {
    // *random_state = seed ^ syscall(__NR_gettid);
    *random_state = seed;
  } else {
    const int key_length = 3;
    uint64_t init_key[key_length];
    struct timeval t1;

    gettimeofday(&t1, NULL);

    /* Hopefully the following seed is good enough for Montercarlo */
    init_key[0] = t1.tv_sec;
    init_key[1] = t1.tv_usec;
    init_key[2] = getpid();

    // *random_state = t1.tv_sec ^ t1.tv_usec ^ getpid();
    *random_state = t1.tv_sec ^ t1.tv_usec ^ syscall(__NR_gettid);
  }
}

/* Output a floating point number r (0.0 <= r < 1.0) */
double generate_random_double(unsigned int *random_state_simple,
                              char mca_rng_mode) {

  if (mca_rng_mode == 1) {
    /* rand */
    int tmp;

    tmp = rand_r(random_state_simple);

    // multiply by 2^-53 = (1.0 / 9007199254740992.0)
    return ((double)(1.0 / 9007199254740992.0) * tmp) / RAND_MAX;
  } else if (mca_rng_mode == 2) {
    /* random - currently unsupported */
    exit(EXIT_FAILURE);
  } else if (mca_rng_mode == 3) {
    /* drand48 - currently unsupported */
    exit(EXIT_FAILURE);
  } else {
    /* unsupported mode */
    exit(EXIT_FAILURE);
  }

  return -1;
}

/* Output a floating point number r (0.0 <= r < 1.0) */
double generate_random_double01(unsigned int *random_state_simple,
                                char mca_rng_mode) {

  return generate_random_double(random_state_simple, mca_rng_mode);
}

/* Output a floating point number r (1.0 <= r < 2.0) */
double generate_random_double12(unsigned int *random_state_simple,
                                char mca_rng_mode) {

  return generate_random_double01(random_state_simple, mca_rng_mode) + 1.0;
}

/* Output a floating point number r (0.0 < r <= 1.0) */
double generate_random_double0C(unsigned int *random_state_simple,
                                char mca_rng_mode) {

  if (mca_rng_mode == 1) {
    /* rand */
    int tmp;

    tmp = rand_r(random_state_simple);
    if (tmp == 0)
      tmp++;

    return ((double)1.0 * tmp) / RAND_MAX;
  } else if (mca_rng_mode == 2) {
    /* random - currently unsupported */
    exit(EXIT_FAILURE);
  } else if (mca_rng_mode == 3) {
    /* drand48 - currently unsupported */
    exit(EXIT_FAILURE);
  } else {
    /* unsupported mode */
    exit(EXIT_FAILURE);
  }

  return -1;
}

/* Output a floating point number r (0.0 < r < 1.0) */
double generate_random_double00(unsigned int *random_state_simple) {
  int tmp = rand_r(random_state_simple);
  
  if (tmp == 0)
    tmp++;
  else if (tmp == RAND_MAX)
    tmp--;

  return ((double)1.0 * tmp) / RAND_MAX;
}
