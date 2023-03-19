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

#ifndef __INTERFLOP_MCAQUAD_H__
#define __INTERFLOP_MCAQUAD_H__

#include "interflop/common/float_const.h"
#include "interflop/interflop.h"
#include "interflop/interflop_stdlib.h"

#define INTERFLOP_MCAQUAD_API(name) interflop_mcaquad_##name

/* define default environment variables and default parameters */
#define MCAQUAD_PRECISION_BINARY32_MIN 1
#define MCAQUAD_PRECISION_BINARY64_MIN 1
#define MCAQUAD_PRECISION_BINARY32_MAX DOUBLE_PMAN_SIZE
#define MCAQUAD_PRECISION_BINARY64_MAX QUAD_PMAN_SIZE
#define MCAQUAD_PRECISION_BINARY32_DEFAULT FLOAT_PREC
#define MCAQUAD_PRECISION_BINARY64_DEFAULT DOUBLE_PREC
#define MCAQUAD_MODE_DEFAULT mcaquad_mode_mca
#define MCAQUAD_ERR_MODE_DEFAULT mcaquad_err_mode_rel
#define MCAQUAD_SEED_DEFAULT 0ULL
#define MCAQUAD_SPARSITY_DEFAULT 1.0f
#define MCAQUAD_ABSOLUTE_ERROR_EXPONENT_DEFAULT 112 // Why 112?
#define MCAQUAD_DAZ_DEFAULT IFalse
#define MCAQUAD_FTZ_DEFAULT IFalse

/* define the available MCA modes of operation */
typedef enum {
  mcaquad_mode_ieee,
  mcaquad_mode_mca,
  mcaquad_mode_pb,
  mcaquad_mode_rr,
  _mcaquad_mode_end_
} mcaquad_mode;

/* define the available error modes */
typedef enum {
  mcaquad_err_mode_rel,
  mcaquad_err_mode_abs,
  mcaquad_err_mode_all,
  _mcaquad_err_mode_end_
} mcaquad_err_mode;

/* Interflop context */
typedef struct {
  IUint64_t seed;
  float sparsity;
  int binary32_precision;
  int binary64_precision;
  int absErr_exp;
  IBool relErr;
  IBool absErr;
  IBool daz;
  IBool ftz;
  IBool choose_seed;
  mcaquad_mode mode;
} mcaquad_context_t;

typedef struct {
  IUint64_t seed;
  float sparsity;
  IUint32_t precision_binary32;
  IUint32_t precision_binary64;
  mcaquad_mode mode;
  mcaquad_err_mode err_mode;
  IInt64_t max_abs_err_exponent;
  IUint32_t daz;
  IUint32_t ftz;
} mcaquad_conf_t;

const char *get_mcaquad_mode_name(mcaquad_mode mode);
void mcaquad_push_seed(IUint64_t);
void mcaquad_pop_seed(void);

void INTERFLOP_MCAQUAD_API(add_float)(float a, float b, float *res,
                                      void *context);
void INTERFLOP_MCAQUAD_API(sub_float)(float a, float b, float *res,
                                      void *context);
void INTERFLOP_MCAQUAD_API(mul_float)(float a, float b, float *res,
                                      void *context);
void INTERFLOP_MCAQUAD_API(div_float)(float a, float b, float *res,
                                      void *context);
void INTERFLOP_MCAQUAD_API(fma_float)(float a, float b, float c, float *res,
                                      void *context);
void INTERFLOP_MCAQUAD_API(add_double)(double a, double b, double *res,
                                       void *context);
void INTERFLOP_MCAQUAD_API(sub_double)(double a, double b, double *res,
                                       void *context);
void INTERFLOP_MCAQUAD_API(mul_double)(double a, double b, double *res,
                                       void *context);
void INTERFLOP_MCAQUAD_API(div_double)(double a, double b, double *res,
                                       void *context);
void INTERFLOP_MCAQUAD_API(fma_double)(double a, double b, double c,
                                       double *res, void *context);
void INTERFLOP_MCAQUAD_API(cast_double_to_float)(double a, float *res,
                                                 void *context);

void INTERFLOP_MCAQUAD_API(user_call)(void *context, interflop_call_id id,
                                      va_list ap);

const char *INTERFLOP_MCAQUAD_API(get_backend_name)(void);
const char *INTERFLOP_MCAQUAD_API(get_backend_version)(void);
void INTERFLOP_MCAQUAD_API(pre_init)(interflop_panic_t panic, File *stream,
                                     void **context);
void INTERFLOP_MCAQUAD_API(cli)(int argc, char **argv, void *context);
void INTERFLOP_MCAQUAD_API(configure)(void *configure, void *context);
struct interflop_backend_interface_t INTERFLOP_MCAQUAD_API(init)(void *context);

#endif /* __INTERFLOP_MCAQUAD_H__ */