/*****************************************************************************\
 *                                                                           *\
 *  This file is part of the Verificarlo project,                            *\
 *  under the Apache License v2.0 with LLVM Exceptions.                      *\
 *  SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception.                 *\
 *  See https://llvm.org/LICENSE.txt for license information.                *\
 *                                                                           *\
 *  Copyright (c) 2019-2023                                                  *\
 *     Verificarlo Contributors                                              *\
 *                                                                           *\
 ****************************************************************************/

#ifndef __INTERFLOP_BITMASK_H__
#define __INTERFLOP_BITMASK_H__

#include "interflop/interflop_stdlib.h"

#define INTERFLOP_BITMASK_API(name) interflop_bitmask_##name

/* define default environment variables and default parameters */
#define BITMASK_PRECISION_BINARY32_MIN 1
#define BITMASK_PRECISION_BINARY64_MIN 1
#define BITMASK_PRECISION_BINARY32_MAX FLOAT_PMAN_SIZE
#define BITMASK_PRECISION_BINARY64_MAX DOUBLE_PMAN_SIZE
#define BITMASK_PRECISION_BINARY32_DEFAULT FLOAT_PMAN_SIZE
#define BITMASK_PRECISION_BINARY64_DEFAULT DOUBLE_PMAN_SIZE
#define BITMASK_OPERATOR_DEFAULT bitmask_operator_zero
#define BITMASK_MODE_DEFAULT bitmask_mode_ob
#define BITMASK_SEED_DEFAULT 0ULL
#define BITMASK_DAZ_DEFAULT IFalse
#define BITMASK_FTZ_DEFAULT IFalse

/* define the available BITMASK modes of operation */
typedef enum {
  bitmask_mode_ieee,
  bitmask_mode_full,
  bitmask_mode_ib,
  bitmask_mode_ob,
  _bitmask_mode_end_
} bitmask_mode;

/* define the available BITMASK */
typedef enum {
  bitmask_operator_zero,
  bitmask_operator_one,
  bitmask_operator_rand,
  _bitmask_operator_end_
} bitmask_operator;

/* Interflop context */
typedef struct {
  IUint64_t seed;
  int binary32_precision;
  int binary64_precision;
  bitmask_operator operator;
  bitmask_mode mode;
  IBool choose_seed;
  IBool daz;
  IBool ftz;
} bitmask_context_t;

typedef bitmask_context_t bitmask_conf_t;

void bitmask_push_seed(IUint64_t seed);
void bitmask_pop_seed(void);

const char *INTERFLOP_BITMASK_API(get_backend_name)(void);
const char *INTERFLOP_BITMASK_API(get_backend_version)(void);

void INTERFLOP_BITMASK_API(add_float)(float a, float b, float *res,
                                      void *context);
void INTERFLOP_BITMASK_API(sub_float)(float a, float b, float *res,
                                      void *context);
void INTERFLOP_BITMASK_API(mul_float)(float a, float b, float *res,
                                      void *context);
void INTERFLOP_BITMASK_API(div_float)(float a, float b, float *res,
                                      void *context);
void INTERFLOP_BITMASK_API(fma_float)(float a, float b, float c, float *res,
                                      void *context);
void INTERFLOP_BITMASK_API(add_double)(double a, double b, double *res,
                                       void *context);
void INTERFLOP_BITMASK_API(sub_double)(double a, double b, double *res,
                                       void *context);
void INTERFLOP_BITMASK_API(mul_double)(double a, double b, double *res,
                                       void *context);
void INTERFLOP_BITMASK_API(div_double)(double a, double b, double *res,
                                       void *context);
void INTERFLOP_BITMASK_API(fma_double)(double a, double b, double c,
                                       double *res, void *context);
void INTERFLOP_BITMASK_API(cast_double_to_float)(double a, float *res,
                                                 void *context);
void INTERFLOP_BITMASK_API(pre_init)(interflop_panic_t panic, File *stream,
                                     void **context);
void INTERFLOP_BITMASK_API(cli)(int argc, char **argv, void *context);
void INTERFLOP_BITMASK_API(configure)(void *configure, void *context);
struct interflop_backend_interface_t INTERFLOP_BITMASK_API(init)(void *context);

#endif /* __INTERFLOP_BITMASK_H__ */