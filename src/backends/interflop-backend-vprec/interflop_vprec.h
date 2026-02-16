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
 *  Copyright (c) 2019-2026                                                  *\
 *     Verificarlo Contributors                                              *\
 *                                                                           *\
 ****************************************************************************/

#ifndef __INTERFLOP_VPREC_H__
#define __INTERFLOP_VPREC_H__

#include "interflop/common/float_const.h"
#include "interflop/iostream/logger.h"
#include "interflop_vprec_function_instrumentation.h"

#define INTERFLOP_VPREC_API(name) interflop_vprec_##name

/* define default environment variables and default parameters */

/* default values of precision and range for binary32 */
#define VPREC_PRECISION_BINARY32_MIN 1
#define VPREC_PRECISION_BINARY32_MAX FLOAT_PMAN_SIZE
#define VPREC_PRECISION_BINARY32_DEFAULT FLOAT_PMAN_SIZE
#define VPREC_RANGE_BINARY32_MIN 2
#define VPREC_RANGE_BINARY32_MAX FLOAT_EXP_SIZE
#define VPREC_RANGE_BINARY32_DEFAULT FLOAT_EXP_SIZE

/* default values of precision and range for binary64 */
#define VPREC_PRECISION_BINARY64_MIN 1
#define VPREC_PRECISION_BINARY64_MAX DOUBLE_PMAN_SIZE
#define VPREC_PRECISION_BINARY64_DEFAULT DOUBLE_PMAN_SIZE
#define VPREC_RANGE_BINARY64_MIN 2
#define VPREC_RANGE_BINARY64_MAX DOUBLE_EXP_SIZE
#define VPREC_RANGE_BINARY64_DEFAULT DOUBLE_EXP_SIZE

/* default mode value  */
#define VPREC_MODE_DEFAULT vprecmode_ob

/* default error mode value */
#define VPREC_ERR_MODE_DEFAULT vprec_err_mode_rel

typedef enum {
  KEY_PREC_B32,
  KEY_PREC_B64,
  KEY_RANGE_B32,
  KEY_RANGE_B64,
  KEY_ERR_EXP,
  KEY_INPUT_FILE,
  KEY_OUTPUT_FILE,
  KEY_LOG_FILE,
  KEY_PRESET,
  KEY_MODE = 'm',
  KEY_ERR_MODE = 'e',
  KEY_INSTRUMENT = 'i',
  KEY_DAZ = 'd',
  KEY_FTZ = 'f'
} key_args;

/* define the available VPREC modes of operation */
typedef enum {
  vprecmode_ieee,
  vprecmode_full,
  vprecmode_ib,
  vprecmode_ob,
  _vprecmode_end_
} vprec_mode;

/* define the available error modes */
typedef enum {
  vprec_err_mode_rel,
  vprec_err_mode_abs,
  vprec_err_mode_all,
  _vprec_err_mode_end_
} vprec_err_mode;

/* define the possible VPREC operation */
typedef enum {
  vprec_add = '+',
  vprec_sub = '-',
  vprec_mul = '*',
  vprec_div = '/',
  vprec_fma = 'f',
} vprec_operation;

/* define the possible VPREC preset */
typedef enum {
  vprec_preset_binary16,
  vprec_preset_binary32,
  vprec_preset_bfloat16,
  vprec_preset_tensorfloat,
  vprec_preset_fp24,
  vprec_preset_PXR24,
  _vprec_preset_end_
} vprec_preset;

typedef enum {
  vprec_preset_precision_binary16 = 10,
  vprec_preset_precision_binary32 = 23,
  vprec_preset_precision_bfloat16 = 7,
  vprec_preset_precision_tensorfloat = 10,
  vprec_preset_precision_fp24 = 16,
  vprec_preset_precision_PXR24 = 15,
  _vprec_preset_precision_end_
} vprec_preset_precision;

typedef enum {
  vprec_preset_range_binary16 = 5,
  vprec_preset_range_binary32 = 8,
  vprec_preset_range_bfloat16 = 8,
  vprec_preset_range_tensorfloat = 8,
  vprec_preset_range_fp24 = 7,
  vprec_preset_range_PXR24 = 8,
  _vprec_preset_range_end_
} vprec_preset_range;

/* Interflop context */
typedef struct {
  /* structure holding vprec function instrumentation variables */
  t_context_vfi *vfi;
  /* arithmetic variables */
  int binary32_precision;
  int binary32_range;
  int binary64_precision;
  int binary64_range;
  int absErr_exp;
  vprec_mode mode;
  IBool relErr;
  IBool absErr;
  IBool daz;
  IBool ftz;
} vprec_context_t;

typedef struct {
  unsigned int precision_binary32;
  unsigned int range_binary32;
  unsigned int precision_binary64;
  unsigned int range_binary64;
  vprec_mode mode;
  vprec_preset preset;
  vprec_err_mode err_mode;
  long max_abs_err_exponent;
  unsigned int daz;
  unsigned int ftz;
} vprec_conf_t;

void _set_vprec_precision_binary32(int precision, vprec_context_t *ctx);
void _set_vprec_range_binary32(int range, vprec_context_t *ctx);
void _set_vprec_precision_binary64(int precision, vprec_context_t *ctx);
void _set_vprec_range_binary64(int range, vprec_context_t *ctx);
float _vprec_round_binary32(float a, char is_input, void *context,
                            int binary32_range, int binary32_precision);
double _vprec_round_binary64(double a, char is_input, void *context,
                             int binary64_range, int binary64_precision);
extern struct argp vfi_argp;

const char *get_vprec_mode_name(vprec_mode mode);

extern void (*vprec_debug_print_op)(int, const char *, const double *,
                                    const double *);
void vprec_set_debug_print_op(void (*printOpHandler)(
    int nbArg, const char *name, const double *args, const double *res));

/* Interflop API */

void INTERFLOP_VPREC_API(add_float)(float a, float b, float *c, void *context);
void INTERFLOP_VPREC_API(sub_float)(float a, float b, float *c, void *context);
void INTERFLOP_VPREC_API(mul_float)(float a, float b, float *c, void *context);
void INTERFLOP_VPREC_API(div_float)(float a, float b, float *c, void *context);
void INTERFLOP_VPREC_API(add_double)(double a, double b, double *c,
                                     void *context);
void INTERFLOP_VPREC_API(sub_double)(double a, double b, double *c,
                                     void *context);
void INTERFLOP_VPREC_API(mul_double)(double a, double b, double *c,
                                     void *context);
void INTERFLOP_VPREC_API(div_double)(double a, double b, double *c,
                                     void *context);
void INTERFLOP_VPREC_API(cast_double_to_float)(double a, float *b,
                                               void *context);
void INTERFLOP_VPREC_API(fma_float)(float a, float b, float c, float *res,
                                    void *context);
void INTERFLOP_VPREC_API(fma_double)(double a, double b, double c, double *res,
                                     void *context);
void INTERFLOP_VPREC_API(enter_function)(interflop_function_stack_t *stack,
                                         void *context, int nb_args,
                                         va_list ap);
void INTERFLOP_VPREC_API(exit_function)(interflop_function_stack_t *stack,
                                        void *context, int nb_args, va_list ap);

void INTERFLOP_VPREC_API(user_call)(void *context, interflop_call_id id,
                                    va_list ap);
void INTERFLOP_VPREC_API(finalize)(void *context);

const char *INTERFLOP_VPREC_API(get_backend_name)(void);
const char *INTERFLOP_VPREC_API(get_backend_version)(void);
void INTERFLOP_VPREC_API(pre_init)(interflop_panic_t panic, File *stream,
                                   void **context);
void INTERFLOP_VPREC_API(cli)(int argc, char **argv, void *context);
void INTERFLOP_VPREC_API(configure)(void *configure, void *context);
struct interflop_backend_interface_t INTERFLOP_VPREC_API(init)(void *context);

#endif /* __INTERFLOP_VPREC_H__ */
