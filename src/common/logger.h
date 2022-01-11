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

/* Display the info message */
void logger_info(const char *fmt, ...);
/* Display the warning message */
void logger_warning(const char *fmt, ...);
/* Display the error message */
void logger_error(const char *fmt, ...);

/* Display the info message */
void vlogger_info(const char *fmt, va_list argp);
/* Display the warning message */
void vlogger_warning(const char *fmt, va_list argp);
/* Display the error message */
void vlogger_error(const char *fmt, va_list argp);

void logger_init(void);

#endif /* __LOGGER_H__ */
