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
 *  Copyright (c) 2019-2022                                                  *\
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

/* Returns a bool for determining whether an operation should skip */
/* perturbation. false -> perturb; true -> skip. */
/* e.g. for sparsity=0.1, all random values > 0.1 = true -> no MCA*/
bool _mca_skip_eval(const float sparsity, rng_state_t *rng_state,
                    pid_t *global_tid) {
  if (sparsity >= 1.0f) {
    return false;
  }

  return (get_rand_double01(rng_state, global_tid) > sparsity);
}
