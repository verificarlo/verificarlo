/*****************************************************************************
 *                                                                           *
 *  This file is part of Verificarlo.                                        *
 *                                                                           *
 *  Copyright (c) 2015                                                       *
 *     Universite de Versailles St-Quentin-en-Yvelines                       *
 *     CMLA, Ecole Normale Superieure de Cachan                              *
 *  Copyright (c) 2018-2020                                                  *
 *     Verificarlo contributors                                              *
 *     Universite de Versailles St-Quentin-en-Yvelines                       *
 *                                                                           *
 *  Verificarlo is free software: you can redistribute it and/or modify      *
 *  it under the terms of the GNU General Public License as published by     *
 *  the Free Software Foundation, either version 3 of the License, or        *
 *  (at your option) any later version.                                      *
 *                                                                           *
 *  Verificarlo is distributed in the hope that it will be useful,           *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *  GNU General Public License for more details.                             *
 *                                                                           *
 *  You should have received a copy of the GNU General Public License        *
 *  along with Verificarlo.  If not, see <http://www.gnu.org/licenses/>.     *
 *                                                                           *
 *****************************************************************************/

#ifndef __LOGGER_H__
#define __LOGGER_H__

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
