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

#ifndef __INTERFLOP_MCAINT_H__
#define __INTERFLOP_MCAINT_H__

#include "interflop/interflop_stdlib.h"

#define INTERFLOP_MCAINT_API(name) interflop_mcaint_##name

/* define default environment variables and default parameters */
#define MCAINT_PRECISION_BINARY32_MIN 1
#define MCAINT_PRECISION_BINARY64_MIN 1
#define MCAINT_PRECISION_BINARY32_MAX DOUBLE_PMAN_SIZE
#define MCAINT_PRECISION_BINARY64_MAX QUAD_PMAN_SIZE
#define MCAINT_PRECISION_BINARY32_DEFAULT FLOAT_PREC
#define MCAINT_PRECISION_BINARY64_DEFAULT DOUBLE_PREC
#define MCAINT_ABSOLUTE_ERROR_EXPONENT_DEFAULT 112 // Why 112?
#define MCAINT_SEED_DEFAULT 0ULL
#define MCAINT_SPARSITY_DEFAULT 1.0f
#define MCAINT_MODE_DEFAULT mcaint_mode_mca
#define MCAINT_ERR_MODE_DEFAULT mcaint_err_mode_rel
#define MCAINT_DAZ_DEFAULT IFalse
#define MCAINT_FTZ_DEFAULT IFalse

/* define the available MCA modes of operation */
typedef enum {
  mcaint_mode_ieee,
  mcaint_mode_mca,
  mcaint_mode_pb,
  mcaint_mode_rr,
  _mcaint_mode_end_
} mcaint_mode;

/* define the available error modes */
typedef enum {
  mcaint_err_mode_rel,
  mcaint_err_mode_abs,
  mcaint_err_mode_all,
  _mcaint_err_mode_end_
} mcaint_err_mode;

/* Interflop context */
typedef struct {
  IBool relErr;
  IBool absErr;
  IBool daz;
  IBool ftz;
  IBool choose_seed;
  mcaint_mode mode;
  int binary32_precision;
  int binary64_precision;
  int absErr_exp;
  float sparsity;
  IUint64_t seed;
} mcaint_context_t;

typedef struct {
  IUint64_t seed;
  float sparsity;
  IUint32_t precision_binary32;
  IUint32_t precision_binary64;
  mcaint_mode mode;
  mcaint_err_mode err_mode;
  IInt64_t max_abs_err_exponent;
  IUint32_t daz;
  IUint32_t ftz;
} mcaint_conf_t;

void mcaint_push_seed(IUint64_t seed);
void mcaint_pop_seed(void);

void INTERFLOP_MCAINT_API(add_float)(float a, float b, float *res,
                                     void *context);
void INTERFLOP_MCAINT_API(sub_float)(float a, float b, float *res,
                                     void *context);
void INTERFLOP_MCAINT_API(mul_float)(float a, float b, float *res,
                                     void *context);
void INTERFLOP_MCAINT_API(div_float)(float a, float b, float *res,
                                     void *context);
void INTERFLOP_MCAINT_API(fma_float)(float a, float b, float c, float *res,
                                     void *context);
void INTERFLOP_MCAINT_API(add_double)(double a, double b, double *res,
                                      void *context);
void INTERFLOP_MCAINT_API(sub_double)(double a, double b, double *res,
                                      void *context);
void INTERFLOP_MCAINT_API(mul_double)(double a, double b, double *res,
                                      void *context);
void INTERFLOP_MCAINT_API(div_double)(double a, double b, double *res,
                                      void *context);
void INTERFLOP_MCAINT_API(fma_double)(double a, double b, double c, double *res,
                                      void *context);
void INTERFLOP_MCAINT_API(cast_double_to_float)(double a, float *res,
                                                void *context);

const char *INTERFLOP_MCAINT_API(get_backend_name)(void);
const char *INTERFLOP_MCAINT_API(get_backend_version)(void);
void INTERFLOP_MCAINT_API(pre_init)(interflop_panic_t panic, File *stream,
                                    void **context);
void INTERFLOP_MCAINT_API(cli)(int argc, char **argv, void *context);
void INTERFLOP_MCAINT_API(configure)(void *configure, void *context);
struct interflop_backend_interface_t INTERFLOP_MCAINT_API(init)(void *context);

#endif /* __INTERFLOP_MCAINT_H__ */