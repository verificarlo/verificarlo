
/*****************************************************************************\
 *                                                                           *\
 *  This file is part of the Verificarlo project,                            *\
 *  under the Apache License v2.0 with LLVM Exceptions.                      *\
 *  SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception.                 *\
 *  See https://llvm.org/LICENSE.txt for license information.                *\
 *                                                                           *\
 *  Copyright (c) 2022                                                  *\
 *     Verificarlo Contributors                                              *\
 *                                                                           *\
 ****************************************************************************/

#include "vfc_rng.h"
#include "interflop_stdlib.h"
#include "splitmix64.h"
#include "xoroshiro128.h"

/* A macro to initialize the initialization of the seed and random state for the
 * random number generator */
/* RANDOM_STATE      is a pointer to the structure that all RNG-related data */
/* GLOBAL_TID        is a pointer to the global TID */
#define _INIT_RANDOM_STATE(RANDOM_STATE, GLOBAL_TID)                           \
  {                                                                            \
    if (RANDOM_STATE->random_state_valid == false) {                           \
      if (RANDOM_STATE->choose_seed == true) {                                 \
        _set_seed(RANDOM_STATE, RANDOM_STATE->choose_seed,                     \
                  RANDOM_STATE->seed ^ _get_new_tid(GLOBAL_TID));              \
      } else {                                                                 \
        _set_seed(RANDOM_STATE, false, 0);                                     \
      }                                                                        \
      RANDOM_STATE->random_state_valid = true;                                 \
    }                                                                          \
  }

/* Generic set_seed function which is common for most of the backends */
/* @param random state pointer to the internal state of the RNG */
/* @param choose_seed whether to set the seed to a user-provided value */
/* @param seed the user-provided seed for the RNG */
static void _set_seed(rng_state_t *random_state, const bool choose_seed,
                      const uint64_t seed) {
  if (choose_seed) {
    random_state->seed = seed;
  } else {
    /* TODO: to optimize seeding with rdseed asm instruction for Intel arch */
    struct timeval t1;
    interflop_gettimeofday(&t1, NULL);
    /* Hopefully the following seed is good enough for Montercarlo */
    random_state->seed = t1.tv_sec ^ t1.tv_usec ^ interflop_gettid();
  }
  random_state->random_state[0] = next_seed(random_state->seed);
  random_state->random_state[1] = next_seed(random_state->seed);
}

/* Get a new identifier for the calling thread */
/* Generic threads can have inconsistent identifiers, assigned by the system, */
/* we therefore need to set an order between threads, for the case */
/* when the seed is fixed, to insure some repeatability between executions */
/* @param global_tid pointer to the unique TID */
/* @return a new unique identifier for each calling thread*/
pid_t _get_new_tid(pid_t *global_tid) {
  return __atomic_add_fetch(global_tid, 1, __ATOMIC_SEQ_CST);
}

/* Initialize a data structure used to hold the information required */
/* by the RNG */
void _init_rng_state_struct(rng_state_t *rng_state, bool choose_seed,
                            uint64_t seed, bool random_state_valid) {
  if (rng_state->random_state_valid == false) {
    rng_state->choose_seed = choose_seed;
    rng_state->seed = seed;
    rng_state->random_state_valid = random_state_valid;
  }
}

/* Returns a 32-bit unsigned integer r (0 <= r < 2^32) */
uint32_t get_rand_uint32(rng_state_t *rng_state, pid_t *global_tid) {
  _INIT_RANDOM_STATE(rng_state, global_tid);
  const union {
    uint64_t u64;
    uint32_t u32[2];
  } u = {.u64 = next(rng_state->random_state)};
  return u.u32[0];
}

/* Returns a 64-bit unsigned integer r (0 <= r < 2^64) */
uint64_t get_rand_uint64(rng_state_t *rng_state, pid_t *global_tid) {
  _INIT_RANDOM_STATE(rng_state, global_tid);
  return next(rng_state->random_state);
}

/* Returns a random double in the (0,1) open interval */
double get_rand_double01(rng_state_t *rng_state, pid_t *global_tid) {
  _INIT_RANDOM_STATE(rng_state, global_tid);
  return next_double(rng_state->random_state);
}
