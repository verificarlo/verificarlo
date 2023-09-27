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

#ifndef __INTERFLOP_STDLIB_H__
#define __INTERFLOP_STDLIB_H__

#include <stdarg.h>

#if defined(__cplusplus)
extern "C" {
#endif

#define Null 0

typedef long unsigned int ISize_t;
typedef int IInt32_t;
typedef unsigned int IUint32_t;
typedef long int IInt64_t;
typedef unsigned long int IUint64_t;
typedef void File;
typedef int IBool;
typedef void Itimeval_t;
typedef void Itimezone_t;

/* IBool */
#define ITrue 1
#define IFalse 0

#define EXIT_FAILURE 1

#define INTERFLOP_CHECK_IMPL(name)                                             \
  if (interflop_##name == Null) {                                              \
    interflop_panic("Interflop backend error: " #name " not implemented\n");   \
  }

typedef void (*interflop_set_handler_t)(const char *name, void *function_ptr);

void interflop_set_handler(const char *name, void *function_ptr);

typedef void *(*interflop_malloc_t)(ISize_t);
typedef File *(*interflop_fopen_t)(const char *pathname, const char *mode,
                                   int *error);
typedef void (*interflop_panic_t)(const char *);
typedef int (*interflop_strcmp_t)(const char *s1, const char *s2);
typedef int (*interflop_strcasecmp_t)(const char *s1, const char *s2);
/* Do not follow libc API, use error pointer to pass errno result instead */
/* error = 0 if success */
typedef long (*interflop_strtol_t)(const char *nptr, char **endptr, int *error);
typedef double (*interflop_strtod_t)(const char *nptr, char **endptr,
                                     int *error);
typedef char *(*interflop_getenv_t)(const char *name);
typedef int (*interflop_fprintf_t)(File *stream, const char *format, ...);
typedef char (*interflop_strcpy_t)(char *dest, const char *src);
typedef int (*interflop_fclose_t)(File *stream);
typedef int (*interflop_gettid_t)(void);
typedef char *(*interflop_strerror_t)(int error);
typedef int (*interflop_sprintf_t)(char *str, const char *format, ...);
typedef void (*interflop_vwarnx_t)(const char *fmt, va_list args);
typedef int (*interflop_vfprintf_t)(File *stream, const char *format,
                                    va_list ap);
typedef void (*interflop_exit_t)(int status);
typedef char *(*interflop_strtok_r_t)(char *str, const char *delim,
                                      char **saveptr);
typedef char *(*interflop_fgets_t)(char *s, int size, File *stream);
typedef void (*interflop_free_t)(void *ptr);
typedef void *(*interflop_calloc_t)(ISize_t nmemb, ISize_t size);
typedef int (*interflop_argp_parse_t)(void *__argp, int __argc, char **__argv,
                                      unsigned int __flags, int *__arg_index,
                                      void *__input);
typedef void (*interflop_nanHandler_t)(void);
typedef void (*interflop_infHandler_t)(void);
typedef void (*interflop_maxHandler_t)(void);
typedef void (*interflop_cancellationHandler_t)(int);
typedef void (*interflop_denormalHandler_t)(void);

typedef void (*interflop_debug_print_op_t)(int, const char *,
                                           const double *args,
                                           const double *res);
typedef int (*interflop_gettimeofday_t)(Itimeval_t *tv, Itimezone_t *tz);

typedef int (*interflop_register_printf_specifier_t)(int __spec, void *__func,
                                                     void *__arginfo);

extern interflop_malloc_t interflop_malloc;
extern interflop_fopen_t interflop_fopen;
extern interflop_panic_t interflop_panic;
extern interflop_strcmp_t interflop_strcmp;
extern interflop_strcasecmp_t interflop_strcasecmp;
extern interflop_strtol_t interflop_strtol;
extern interflop_strtod_t interflop_strtod;
extern interflop_getenv_t interflop_getenv;
extern interflop_fprintf_t interflop_fprintf;
extern interflop_strcpy_t interflop_strcpy;
extern interflop_fclose_t interflop_fclose;
extern interflop_gettid_t interflop_gettid;
extern interflop_strerror_t interflop_strerror;
extern interflop_sprintf_t interflop_sprintf;
extern interflop_vwarnx_t interflop_vwarnx;
extern interflop_vfprintf_t interflop_vfprintf;
extern interflop_exit_t interflop_exit;
extern interflop_strtok_r_t interflop_strtok_r;
extern interflop_fgets_t interflop_fgets;
extern interflop_free_t interflop_free;
extern interflop_calloc_t interflop_calloc;
extern interflop_argp_parse_t interflop_argp_parse;
extern interflop_nanHandler_t interflop_nanHandler;
extern interflop_infHandler_t interflop_infHandler;
extern interflop_maxHandler_t interflop_maxHandler;
extern interflop_cancellationHandler_t interflop_cancellationHandler;
extern interflop_denormalHandler_t interflop_denormalHandler;
extern interflop_debug_print_op_t interflop_debug_print_op;
extern interflop_gettimeofday_t interflop_gettimeofday;
extern interflop_register_printf_specifier_t
    interflop_register_printf_specifier;

float fpow2i(int i);
double pow2i(int i);
int interflop_isnanf(float x);
int interflop_isnand(double x);
int interflop_isinff(float x);
int interflop_isinfd(double x);
float interflop_floorf(float x);
double interflop_floord(double x);
float interflop_ceilf(float x);
double interflop_ceild(double x);

#define interflop_fpclassify(x)                                                \
  _Generic(x, float : interflop_fpclassifyf, double : interflop_fpclassifyd)(x)

#define interflop_isnan(x)                                                     \
  _Generic(x, float : interflop_isnanf, double : interflop_isnand)(x)
#define interflop_isinf(x)                                                     \
  _Generic(x, float : interflop_isinff, double : interflop_isinfd)(x)

#define interflop_floor(x)                                                     \
  _Generic(x, float : interflop_floorf, double : interflop_floord)(x)

#define interflop_ceil(x)                                                      \
  _Generic(x, float : interflop_ceilf, double : interflop_ceild)(x)

#if defined(__cplusplus)
}
#endif

#endif /* __INTERFLOP_STDLIB_H__ */