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

#ifndef __VFC_RNG_H__
#define __VFC_RNG_H__

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <unistd.h>

#include "xoroshiro128.h"
#define __INTERNAL_RNG_STATE xoroshiro_state

/* Data type used to hold information required by the RNG */
typedef struct rng_state {
  bool choose_seed;
  uint64_t seed;
  bool random_state_valid;
  __INTERNAL_RNG_STATE random_state;
} rng_state_t;

/* Get a new identifier for the calling thread */
/* Generic threads can have inconsistent identifiers, assigned by the system, */
/* we therefore need to set an order between threads, for the case */
/* when the seed is fixed, to insure some repeatability between executions */
/* @param global_tid pointer to the unique TID */
/* @return a new unique identifier for each calling thread*/
pid_t _get_new_tid(pid_t *global_tid);

/* Initialize the data structure used to hold the information required */
/* by the RNG */
/* @param rng_state pointer to the structure holding all the RNG-related data */
/* @param choose_seed whether to set the seed to a user-provided value */
/* @param seed the user-provided seed for the RNG */
/* @param random_state_valid whether RNG internal state has been initialized */
void _init_rng_state_struct(rng_state_t *rng_state, bool choose_seed,
                            uint64_t seed, bool random_state_valid);

/* Returns a 64-bit unsigned integer r (0 <= r < 2^64) */
/* Manages the internal state of the RNG, if necessary */
/* @param rng_state pointer to the structure holding all the RNG-related data */
/* @param global_tid pointer to the unique TID */
/* @return a 64-bit unsigned integer r (0 <= r < 2^64) */
uint64_t get_rand_uint64(rng_state_t *rng_state, pid_t *global_tid);

/* Returns a 32-bit unsigned integer r (0 <= r < 2^32) */
/* Manages the internal state of the RNG, if necessary */
/* @param rng_state pointer to the structure holding all the RNG-related data */
/* @param global_tid pointer to the unique TID */
/* @return a 32-bit unsigned integer r (0 <= r < 2^32) */
uint32_t get_rand_uint32(rng_state_t *rng_state, pid_t *global_tid);

/* Returns a random double in the (0,1) open interval */
/* Manages the internal state of the RNG, if necessary */
/* @param rng_state pointer to the structure holding all the RNG-related data */
/* @param global_tid pointer to the unique TID */
/* @return a floating point number r (0.0 < r < 1.0) */
double get_rand_double01(rng_state_t *rng_state, pid_t *global_tid);

#endif /* __VFC_RNG_H__ */
