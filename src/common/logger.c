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
#include <err.h>
#include <errno.h>
#include <locale.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

#define str(X) #X
#define xstr(X) str(X)

#ifndef BACKEND_HEADER
#define BACKEND_HEADER ""
#endif

#define BACKEND_HEADER_STR xstr(BACKEND_HEADER)

static const char backend_header[] = BACKEND_HEADER_STR;

/* ANSI colors */
typedef enum {
  red = 0,
  green,
  yellow,
  blue,
  magenta,
  cyan,
  bold_red,
  bold_green,
  bold_yellow,
  bold_blue,
  bold_magenta,
  bold_cyan,
  reset
} ansi_colors_t;

/* ANSI escape sequences for normal colors */
static const char ansi_color_red[] = "\x1b[31m";
static const char ansi_color_green[] = "\x1b[32m";
static const char ansi_color_yellow[] = "\x1b[33m";
static const char ansi_color_blue[] = "\x1b[34m";
static const char ansi_color_magenta[] = "\x1b[35m";
static const char ansi_color_cyan[] = "\x1b[36m";
static const char ansi_color_reset[] = "\x1b[0m";

/* ANSI escape sequences for bold colors */
static const char ansi_color_bold_red[] = "\x1b[1;31m";
static const char ansi_color_bold_green[] = "\x1b[1;32m";
static const char ansi_color_bold_yellow[] = "\x1b[1;33m";
static const char ansi_color_bold_blue[] = "\x1b[1;34m";
static const char ansi_color_bold_magenta[] = "\x1b[1;35m";
static const char ansi_color_bold_cyan[] = "\x1b[1;36m";

/* Array of ANSI colors */
/* The order of the colors must be the same than the ansi_colors_t one */
static const char *ansi_colors[] = {
    ansi_color_red,       ansi_color_green,        ansi_color_yellow,
    ansi_color_blue,      ansi_color_magenta,      ansi_color_cyan,
    ansi_color_bold_red,  ansi_color_bold_green,   ansi_color_bold_yellow,
    ansi_color_bold_blue, ansi_color_bold_magenta, ansi_color_bold_cyan,
    ansi_color_reset};

/* Define the color of each level */
typedef enum {
  backend_color = green,
  info_color = bold_blue,
  warning_color = bold_yellow,
  error_color = bold_red,
  reset_color = reset
} level_color;

/* Environment variable for enabling/disabling the logger */
static const char vfc_backends_logger[] = "VFC_BACKENDS_LOGGER";

/* Environment variable for specifying the verificarlo logger output file */
static const char vfc_backends_logfile[] = "VFC_BACKENDS_LOGFILE";

/* Environment variable for enabling/disabling the color */
static const char vfc_backends_colored_logger[] = "VFC_BACKENDS_COLORED_LOGGER";

static bool logger_enabled = true;
static bool logger_colored = false;
static FILE *logger_logfile = NULL;

/* Returns true if the logger is enabled */
bool is_logger_enabled(void) {
  const char *is_logger_enabled_env = getenv(vfc_backends_logger);
  if (is_logger_enabled_env == NULL) {
    return true;
  } else if (strcasecmp(is_logger_enabled_env, "True") == 0) {
    return true;
  } else {
    return false;
  }
}

/* Returns true if the color is enabled */
bool is_logger_colored(void) {
  const char *is_colored_logger_env = getenv(vfc_backends_colored_logger);
  if (is_colored_logger_env == NULL) {
    return false;
  } else if (strcasecmp(is_colored_logger_env, "True") == 0) {
    return true;
  } else {
    return false;
  }
}

pid_t get_tid() { return syscall(__NR_gettid); }

void _error() {
  int errsv = errno;
  locale_t locale = NULL;
  char *msg = strerror_l(errsv, locale);
  err(EXIT_FAILURE, "Error [%s]: %s", BACKEND_HEADER_STR, msg);
}

void set_logger_logfile(void) {
  if (logger_logfile != NULL) {
    return;
  }
  const char *logger_logfile_env = getenv(vfc_backends_logfile);
  if (logger_logfile_env == NULL) {
    logger_logfile = stdout;
  } else {
    /* Create log file specific to TID to avoid non-deterministic output */
    char tmp[1024];
    sprintf(tmp, "%s.%d", logger_logfile_env, get_tid());
    logger_logfile = fopen(tmp, "a");
    if (logger_logfile == NULL) {
      _error();
    }
  }
}

void logger_header(FILE *stream, const char *lvl_name,
                   const level_color lvl_color, const bool colored) {
  if (colored) {
    fprintf(stream, "%s%s%s [%s%s%s]: ", ansi_colors[lvl_color], lvl_name,
            ansi_colors[reset_color], ansi_colors[backend_color],
            BACKEND_HEADER_STR, ansi_colors[reset_color]);
  } else {
    fprintf(stream, "%s [%s]: ", lvl_name, backend_header);
  }
}

/* Display the info message */
void logger_info(const char *fmt, ...) {
  if (logger_enabled) {
    logger_header(logger_logfile, "Info", info_color, logger_colored);
    va_list ap;
    va_start(ap, fmt);
    vfprintf(logger_logfile, fmt, ap);
    va_end(ap);
  }
}

/* Display the warning message */
void logger_warning(const char *fmt, ...) {
  if (logger_enabled) {
    logger_header(stderr, "Warning", warning_color, logger_colored);
  }
  va_list ap;
  va_start(ap, fmt);
  vwarnx(fmt, ap);
  va_end(ap);
}

/* Display the error message */
void logger_error(const char *fmt, ...) {
  if (logger_enabled) {
    logger_header(stderr, "Error", error_color, logger_colored);
  }
  va_list ap;
  va_start(ap, fmt);
  verrx(EXIT_FAILURE, fmt, ap);
  va_end(ap);
}

/* Display the info message */
void vlogger_info(const char *fmt, va_list argp) {
  if (logger_enabled) {
    logger_header(logger_logfile, "Info", info_color, logger_colored);
    vfprintf(logger_logfile, fmt, argp);
  }
}

/* Display the warning message */
void vlogger_warning(const char *fmt, va_list argp) {
  if (logger_enabled) {
    logger_header(stderr, "Warning", warning_color, logger_colored);
  }
  vwarnx(fmt, argp);
}

/* Display the error message */
void vlogger_error(const char *fmt, va_list argp) {
  if (logger_enabled) {
    logger_header(stderr, "Error", error_color, logger_colored);
  }
  verrx(EXIT_FAILURE, fmt, argp);
}

void logger_init(void) {
  logger_enabled = is_logger_enabled();
  logger_colored = is_logger_colored();
  set_logger_logfile();
}
