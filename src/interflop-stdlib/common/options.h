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
#ifndef __OPTIONS_H__
#define __OPTIONS_H__

#include <pthread.h>

#include "interflop/common/float_const.h"
#include "interflop/iostream/logger.h"
#include "interflop/rng/vfc_rng.h"

/* A macro to simplify the generation of calls for interflop hook functions */
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
    T = PRECISION;                                                             \
  }

/* Returns a bool for determining whether an operation should skip */
/* perturbation. false -> perturb; true -> skip. */
/* e.g. for sparsity=0.1, all random values > 0.1 = true -> no MCA*/
/* @param sparsity sparsity */
/* @param rng_state pointer to the structure holding all the RNG-related data */
/* @return false -> perturb; true -> skip */
static bool _mca_skip_eval(const float sparsity, rng_state_t *rng_state,
                           pid_t *global_tid) {
  if (sparsity >= 1.0f) {
    return false;
  }

  return (get_rand_double01(rng_state, global_tid) > sparsity);
}

#endif /* __OPTIONS_H__ */
