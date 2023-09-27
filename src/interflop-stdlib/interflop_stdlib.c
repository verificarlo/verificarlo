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
 *  Copyright (c) 2022                                                       *\
 *     Verificarlo Contributors                                              *\
 *                                                                           *\
 ****************************************************************************/

#include "interflop_stdlib.h"

static int __string_equal(const char *str1, const char *str2) {
  const int length_limit = 2048;

  if (str1 == Null || str2 == Null) {
    return 0;
  }

  int i = 0;
  while (str1[i] == str2[i] && str1[i] != '\0' && str2[i] != '\0' &&
         i <= length_limit) {
    i++;
  }

  if (str1[i] == '\0' && str2[i] == '\0') {
    return 1;
  } else {
    return 0;
  }
}

#define SET_HANDLER(F)                                                         \
  else if (__string_equal(name, #F)) interflop_##F =                           \
      (interflop_##F##_t)function_ptr;

interflop_malloc_t interflop_malloc = Null;
interflop_fopen_t interflop_fopen = Null;
interflop_panic_t interflop_panic = Null;
interflop_strcmp_t interflop_strcmp = Null;
interflop_strcasecmp_t interflop_strcasecmp = Null;
interflop_strtol_t interflop_strtol = Null;
interflop_strtod_t interflop_strtod = Null;
interflop_getenv_t interflop_getenv = Null;
interflop_fprintf_t interflop_fprintf = Null;
interflop_strcpy_t interflop_strcpy = Null;
interflop_fclose_t interflop_fclose = Null;
interflop_gettid_t interflop_gettid = Null;
interflop_strerror_t interflop_strerror = Null;
interflop_sprintf_t interflop_sprintf = Null;
interflop_vwarnx_t interflop_vwarnx = Null;
interflop_vfprintf_t interflop_vfprintf = Null;
interflop_exit_t interflop_exit = Null;
interflop_strtok_r_t interflop_strtok_r = Null;
interflop_fgets_t interflop_fgets = Null;
interflop_free_t interflop_free = Null;
interflop_calloc_t interflop_calloc = Null;
interflop_argp_parse_t interflop_argp_parse = Null;
interflop_nanHandler_t interflop_nanHandler = Null;
interflop_infHandler_t interflop_infHandler = Null;
interflop_maxHandler_t interflop_maxHandler = Null;
interflop_cancellationHandler_t interflop_cancellationHandler = Null;
interflop_denormalHandler_t interflop_denormalHandler = Null;
interflop_debug_print_op_t interflop_debug_print_op = Null;
interflop_gettimeofday_t interflop_gettimeofday = Null;
interflop_register_printf_specifier_t interflop_register_printf_specifier =
    Null;

void interflop_set_handler(const char *name, void *function_ptr) {
  if (name == Null) {
    return;
  }
  SET_HANDLER(malloc)
  SET_HANDLER(fopen)
  SET_HANDLER(panic)
  SET_HANDLER(strcmp)
  SET_HANDLER(strcasecmp)
  SET_HANDLER(strtol)
  SET_HANDLER(strtod)
  SET_HANDLER(getenv)
  SET_HANDLER(fprintf)
  SET_HANDLER(strcpy)
  SET_HANDLER(fclose)
  SET_HANDLER(gettid)
  SET_HANDLER(strerror)
  SET_HANDLER(sprintf)
  SET_HANDLER(vwarnx)
  SET_HANDLER(vfprintf)
  SET_HANDLER(exit)
  SET_HANDLER(strtok_r)
  SET_HANDLER(fgets)
  SET_HANDLER(free)
  SET_HANDLER(calloc)
  SET_HANDLER(argp_parse)
  SET_HANDLER(nanHandler)
  SET_HANDLER(infHandler)
  SET_HANDLER(maxHandler)
  SET_HANDLER(cancellationHandler)
  SET_HANDLER(denormalHandler)
  SET_HANDLER(debug_print_op)
  SET_HANDLER(gettimeofday)
  SET_HANDLER(register_printf_specifier)
}

#include "common/float_const.h"

/* lib math */

typedef union {
  int i;
  float f;
} interflop_b32;

typedef union {
  long i;
  double d;
} interflop_b64;

typedef enum {
  IFP_NAN,
  IFP_INFINITE,
  IFP_ZERO,
  IFP_SUBNORMAL,
  IFS_NORMAL,
} interflop_fpclassify_e;

float interflop_fpclassifyf(float x) {
  interflop_b32 u = {.f = x};
  int exp = u.i & FLOAT_GET_EXP;
  int mant = u.i & FLOAT_GET_PMAN;
  if (exp == 0 && mant == 0) {
    return IFP_ZERO;
  } else if (exp == FLOAT_EXP_MAX && mant == 0) {
    return IFP_INFINITE;
  } else if (exp == FLOAT_EXP_MAX && mant != 0) {
    return IFP_NAN;
  } else if (exp == 0 && mant != 0) {
    return IFP_SUBNORMAL;
  } else {
    return IFS_NORMAL;
  }
}

double interflop_fpclassifyd(double x) {
  interflop_b64 u = {.d = x};
  long exp = u.i & DOUBLE_GET_EXP;
  long mant = u.i & DOUBLE_GET_PMAN;
  if (exp == 0 && mant == 0) {
    return IFP_ZERO;
  } else if (exp == FLOAT_EXP_MAX && mant == 0) {
    return IFP_INFINITE;
  } else if (exp == FLOAT_EXP_MAX && mant != 0) {
    return IFP_NAN;
  } else if (exp == 0 && mant != 0) {
    return IFP_SUBNORMAL;
  } else {
    return IFS_NORMAL;
  }
}

/* returns 2**i convert to a float */
float fpow2i(int i) {
  interflop_b32 x = {.f = 0};

  /* 2**-149 <= result <= 2**127 */
  int exp = i + FLOAT_EXP_COMP;
  if (exp <= -FLOAT_PMAN_SIZE) {
    /* underflow */
    x.f = 0.0f;
  } else if (exp >= FLOAT_EXP_INF) {
    /* overflow */
    x.i = FLOAT_PLUS_INF;
  } else if (exp <= 0) {
    /* subnormal result */
    /* -149 + 127 = -22 */
    x.i = 1 << (FLOAT_PMAN_SIZE - 1 + exp);
  } else {
    /* normal result */
    x.i = exp << FLOAT_PMAN_SIZE;
  }
  return x.f;
}

/* returns 2**i convert to a double */
double pow2i(int i) {
  interflop_b64 x = {.d = 0};

  /* 2**-1074 <= result <= 2**1023 */
  long int exp = i + DOUBLE_EXP_COMP;
  if (exp <= -DOUBLE_PMAN_SIZE) {
    /* underflow */
    x.d = 0.0;
  } else if (exp >= DOUBLE_EXP_INF) {
    /* overflow */
    x.i = DOUBLE_PLUS_INF;
  } else if (exp <= 0) {
    /* subnormal result */
    /* -1074 + 1023 = -51 */
    x.i = 1L << ((long int)DOUBLE_PMAN_SIZE - 1L + exp);
  } else {
    /* normal result */
    x.i = exp << DOUBLE_PMAN_SIZE;
  }
  return x.d;
}

int interflop_isnanf(float x) { return interflop_fpclassifyf(x) == IFP_NAN; }

int interflop_isnand(double x) { return interflop_fpclassifyd(x) == IFP_NAN; }

int interflop_isinff(float x) {
  return interflop_fpclassifyf(x) == IFP_INFINITE;
}

int interflop_isinfd(double x) {
  return interflop_fpclassifyd(x) == IFP_INFINITE;
}

float interflop_floorf(float x) { return __builtin_floor(x); }

double interflop_floord(double x) { return __builtin_floor(x); }

float interflop_ceilf(float x) { return __builtin_ceil(x); }

double interflop_ceild(double x) { return __builtin_ceil(x); }
