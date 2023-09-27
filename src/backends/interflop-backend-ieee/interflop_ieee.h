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

#ifndef __INTERFLOP_IEEE_H__
#define __INTERFLOP_IEEE_H__

#include "interflop/interflop_stdlib.h"

#define INTERFLOP_IEEE_API(name) interflop_ieee_##name

/* Interflop context */
typedef struct {
  IUint64_t mul_count;
  IUint64_t div_count;
  IUint64_t add_count;
  IUint64_t sub_count;
  IUint64_t fma_count;
  IBool debug;
  IBool debug_binary;
  IBool no_backend_name;
  IBool print_new_line;
  IBool print_subnormal_normalized;
  IBool count_op;
} ieee_context_t;

typedef ieee_context_t ieee_conf_t;

void INTERFLOP_IEEE_API(add_float)(const float a, const float b, float *c,
                                   void *context);
void INTERFLOP_IEEE_API(sub_float)(const float a, const float b, float *c,
                                   void *context);
void INTERFLOP_IEEE_API(mul_float)(const float a, const float b, float *c,
                                   void *context);
void INTERFLOP_IEEE_API(div_float)(const float a, const float b, float *c,
                                   void *context);
void INTERFLOP_IEEE_API(cmp_float)(const enum FCMP_PREDICATE p, const float a,
                                   const float b, int *c, void *context);
void INTERFLOP_IEEE_API(add_double)(const double a, const double b, double *c,
                                    void *context);
void INTERFLOP_IEEE_API(sub_double)(const double a, const double b, double *c,
                                    void *context);
void INTERFLOP_IEEE_API(mul_double)(const double a, const double b, double *c,
                                    void *context);
void INTERFLOP_IEEE_API(div_double)(const double a, const double b, double *c,
                                    void *context);
void INTERFLOP_IEEE_API(cmp_double)(const enum FCMP_PREDICATE p, const double a,
                                    const double b, int *c, void *context);
void INTERFLOP_IEEE_API(cast_double_to_float)(double a, float *b,
                                              void *context);
void INTERFLOP_IEEE_API(fma_float)(float a, float b, float c, float *res,
                                   void *context);
void INTERFLOP_IEEE_API(fma_double)(double a, double b, double c, double *res,
                                    void *context);
void INTERFLOP_IEEE_API(finalize)(void *context);

const char *INTERFLOP_IEEE_API(get_backend_name)(void);
const char *INTERFLOP_IEEE_API(get_backend_version)(void);
void INTERFLOP_IEEE_API(pre_init)(interflop_panic_t panic, File *stream,
                                  void **context);
void INTERFLOP_IEEE_API(cli)(int argc, char **argv, void *context);
void INTERFLOP_IEEE_API(configure)(void *configure, void *context);
struct interflop_backend_interface_t INTERFLOP_IEEE_API(init)(void *context);

#endif /* __INTERFLOP_IEEE_H__ */