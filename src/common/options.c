/*****************************************************************************\
 *                                                                           *\
 *  This file is part of the Verificarlo project,                            *\
 *  under the Apache License v2.0 with LLVM Exceptions.                      *\
 *  SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception.                 *\
 *  See https://llvm.org/LICENSE.txt for license information.                *\
 *                                                                           *\
 *                                                                           *\
 *  Copyright (c) 2015                                                       *\
 *     Universite de Versailles St-Quentin-en-Yvelines                       *\
 *     CMLA, Ecole Normale Superieure de Cachan                              *\
 *                                                                           *\
 *  Copyright (c) 2018                                                       *\
 *     Universite de Versailles St-Quentin-en-Yvelines                       *\
 *                                                                           *\
 *  Copyright (c) 2019-2021                                                  *\
 *     Verificarlo Contributors                                              *\
 *                                                                           *\
 ****************************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h> // for getting the thread id
#include <sys/time.h>
#include <sys/types.h>
/* Modern solution for working with threads, since C11 */
/*  currently, support for threads.h is available from glibc2.28 onwards */
// #include <threads.h>
#include <unistd.h>

#include "options.h"

/* A macro to initialize the initialization of the seed and random state for the
 * random number generator */
/* RNG_STATE      is a pointer to the structure that all RNG-related data */
/* GLB_TID_LOCK   is a pointer to the mutex for the acces to the global unique
 * TID */
/* GLB_TID        is a pointer to the global TID */
#define _INIT_RANDOM_STATE(RNG_STATE, GLB_TID_LOCK, GLB_TID)                   \
  {                                                                            \
    if (RNG_STATE->random_state_valid == false) {                              \
      if (RNG_STATE->choose_seed == true) {                                    \
        _set_seed(&(RNG_STATE->random_state), RNG_STATE->choose_seed,          \
                  RNG_STATE->seed ^ _get_new_tid(GLB_TID_LOCK, GLB_TID));      \
      } else {                                                                 \
        _set_seed(&(RNG_STATE->random_state), false, 0);                       \
      }                                                                        \
      RNG_STATE->random_state_valid = true;                                    \
    }                                                                          \
  }

/* Generic set_seed function which is common for most of the backends */
/* @param random state pointer to the internal state of the RNG */
/* @param choose_seed whether to set the seed to a user-provided value */
/* @param seed the user-provided seed for the RNG */
static void _set_seed(struct drand48_data *random_state, const bool choose_seed,
                      const unsigned long long int seed);

/* Outputs 64-bit unsigned integer r (0 <= r < 2^64) */
/* @param random state pointer to the internal state of the RNG */
/* @return a 64-bit unsigned integer r (0 <= r < 2^64) */
static uint64_t _generate_random_uint64(struct drand48_data *random);

/* Output a floating point number r (0.0 < r < 1.0) */
/* @param random state pointer to the internal state of the RNG */
/* @return a floating point number r (0.0 < r < 1.0) */
static double _generate_random_double(struct drand48_data *random_state);

/* Generic set_seed function which is common for most of the backends */
static void _set_seed(struct drand48_data *random_state, const bool choose_seed,
                      const unsigned long long int seed) {
  if (choose_seed) {
    srand48_r((unsigned long int)seed, random_state);
  } else {
    struct timeval t1;
    unsigned long long int tmp_seed;
    unsigned short int tmp_seed_vect[3];

    gettimeofday(&t1, NULL);

    /* Hopefully the following seed is good enough for Montercarlo */
    tmp_seed = t1.tv_sec ^ t1.tv_usec ^ syscall(__NR_gettid);
    tmp_seed_vect[0] = (unsigned short int)(tmp_seed & 0x000000000000FFFF);
    tmp_seed_vect[1] =
        (unsigned short int)((tmp_seed & 0x00000000FFFF0000) >> 16);
    tmp_seed_vect[2] =
        (unsigned short int)((tmp_seed & 0x0000FFFF00000000) >> 32);
    seed48_r(tmp_seed_vect, random_state);
    /* Modern solution for working with threads, since C11 */
    // tmp_seed = t1.tv_sec ^ t1.tv_usec ^ thrd_current();
  }
}

/* Outputs 64-bit unsigned integer r (0 <= r < 2^64) */
static uint64_t _generate_random_uint64(struct drand48_data *random_state) {
  uint64_t tmp_rand1, tmp_rand2, tmp_rand;

  lrand48_r(random_state, &tmp_rand1);
  lrand48_r(random_state, &tmp_rand2);
  tmp_rand = (tmp_rand1 << 32) + tmp_rand2;

  return tmp_rand;
}

/* Output a floating point number r (0.0 < r < 1.0) */
static double _generate_random_double(struct drand48_data *random_state) {
  double tmp_rand;

  drand48_r(random_state, &tmp_rand);
  while (tmp_rand == 0)
    drand48_r(random_state, &tmp_rand);

  return tmp_rand;
}

/* Initialize a data structure used to hold the information required */
/* by the RNG */
void _init_rng_state_struct(rng_state_t *rng_state, bool choose_seed,
                            unsigned long long int seed,
                            bool random_state_valid) {
  if (rng_state->random_state_valid == false) {
    rng_state->choose_seed = choose_seed;
    rng_state->seed = seed;
    rng_state->random_state_valid = random_state_valid;
  }
}

/* Get a new identifier for the calling thread */
/* Generic threads can have inconsistent identifiers, assigned by the system, */
/* we therefore need to set an order between threads, for the case
/* when the seed is fixed, to insure some repeatability between executions */
unsigned long long int _get_new_tid(pthread_mutex_t *global_tid_lock,
                                    unsigned long long int *global_tid) {
  unsigned long long int tmp_tid = -1;

  pthread_mutex_lock(global_tid_lock);
  tmp_tid = (*global_tid)++;
  pthread_mutex_unlock(global_tid_lock);

  return tmp_tid;
}

/* Returns a 64-bit unsigned integer r (0 <= r < 2^64) */
uint64_t _get_rand_uint64(rng_state_t *rng_state,
                          pthread_mutex_t *global_tid_lock,
                          unsigned long long int *global_tid) {
  _INIT_RANDOM_STATE(rng_state, global_tid_lock, global_tid);
  return _generate_random_uint64(&(rng_state->random_state));
}

/* Returns a random double in the (0,1) open interval */
double _get_rand(rng_state_t *rng_state, pthread_mutex_t *global_tid_lock,
                 unsigned long long int *global_tid) {
  _INIT_RANDOM_STATE(rng_state, global_tid_lock, global_tid);
  return _generate_random_double(&(rng_state->random_state));
}

/* Returns a bool for determining whether an operation should skip */
/* perturbation. false -> perturb; true -> skip. */
/* e.g. for sparsity=0.1, all random values > 0.1 = true -> no MCA*/
bool _mca_skip_eval(const float sparsity, rng_state_t *rng_state,
                    pthread_mutex_t *global_tid_lock,
                    unsigned long long int *global_tid) {
  if (sparsity >= 1.0f) {
    return false;
  }

  return (_get_rand(rng_state, global_tid_lock, global_tid) > sparsity);
}
