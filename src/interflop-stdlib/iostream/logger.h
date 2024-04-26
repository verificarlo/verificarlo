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
#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <stdarg.h>

#ifdef __INTERFLOP_BOOTSTRAP__
/* Must be included without the path
 * since interflop-stdlib is not installed yet */
#include "interflop_stdlib.h"
#else
#include "interflop/interflop_stdlib.h"
#endif

#if defined(__cplusplus)
extern "C" {
#endif

/* Display the debug message */
void logger_debug(const char *fmt, ...);
/* Display the info message */
void logger_info(const char *fmt, ...);
/* Display the warning message */
void logger_warning(const char *fmt, ...);
/* Display the error message */
void logger_error(const char *fmt, ...);

/* Display the debug message */
void vlogger_debug(const char *fmt, va_list argp);
/* Display the info message */
void vlogger_info(const char *fmt, va_list argp);
/* Display the warning message */
void vlogger_warning(const char *fmt, va_list argp);
/* Display the error message */
void vlogger_error(const char *fmt, va_list argp);

void logger_init(interflop_panic_t panic, File *stream,
                 const char *backend_name);

#if defined(__cplusplus)
}
#endif

#endif /* __LOGGER_H__ */
