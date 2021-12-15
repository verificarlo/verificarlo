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
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#define str(X) #X
#define xstr(X) str(X)

#ifndef BACKEND_HEADER
#define BACKEND_HEADER ""
#endif

#define BACKEND_HEADER_STR xstr(BACKEND_HEADER)

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

/* Environment variable for enabling/disabling the color */
static const char vfc_backends_colored_logger[] = "VFC_BACKENDS_COLORED_LOGGER";

static bool logger_enabled = true;
static bool logger_colored = false;

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

/* Display the info message */
void logger_info(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  if (logger_enabled) {
    if (logger_colored) {
      fprintf(stdout, "%sInfo%s [%s%s%s]: ", ansi_colors[info_color],
              ansi_colors[reset_color], ansi_colors[backend_color],
              BACKEND_HEADER_STR, ansi_colors[reset_color]);
    } else {
      fprintf(stdout, "Info [%s]: ", BACKEND_HEADER_STR);
    }
    vfprintf(stdout, fmt, ap);
  }
}

/* Display the warning message */
void logger_warning(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  if (logger_enabled) {
    if (logger_colored) {
      fprintf(stderr, "%sWarning%s [%s%s%s]: ", ansi_colors[warning_color],
              ansi_colors[reset_color], ansi_colors[backend_color],
              BACKEND_HEADER_STR, ansi_colors[reset_color]);
    } else {
      fprintf(stderr, "Warning [%s]: ", BACKEND_HEADER_STR);
    }
  }
  vwarnx(fmt, ap);
}

/* Display the error message */
void logger_error(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  if (logger_enabled) {
    if (logger_colored) {
      fprintf(stderr, "%sError%s [%s%s%s]: ", ansi_colors[error_color],
              ansi_colors[reset_color], ansi_colors[backend_color],
              BACKEND_HEADER_STR, ansi_colors[reset_color]);
    } else {
      fprintf(stderr, "Error [%s]: ", BACKEND_HEADER_STR);
    }
  }
  verrx(EXIT_FAILURE, fmt, ap);
}

/* Display the info message */
void vlogger_info(const char *fmt, va_list argp) {
  if (logger_enabled) {
    if (logger_colored) {
      fprintf(stdout, "%sInfo%s [%s%s%s]: ", ansi_colors[info_color],
              ansi_colors[reset_color], ansi_colors[backend_color],
              BACKEND_HEADER_STR, ansi_colors[reset_color]);
    } else {
      fprintf(stdout, "Info [%s]: ", BACKEND_HEADER_STR);
    }
    vfprintf(stdout, fmt, argp);
  }
}

/* Display the warning message */
void vlogger_warning(const char *fmt, va_list argp) {
  if (logger_enabled) {
    if (logger_colored) {
      fprintf(stderr, "%sWarning%s [%s%s%s]: ", ansi_colors[warning_color],
              ansi_colors[reset_color], ansi_colors[backend_color],
              BACKEND_HEADER_STR, ansi_colors[reset_color]);
    } else {
      fprintf(stderr, "Warning [%s]: ", BACKEND_HEADER_STR);
    }
  }
  vwarnx(fmt, argp);
}

/* Display the error message */
void vlogger_error(const char *fmt, va_list argp) {
  if (logger_enabled) {
    if (logger_colored) {
      fprintf(stderr, "%sError%s [%s%s%s]: ", ansi_colors[error_color],
              ansi_colors[reset_color], ansi_colors[backend_color],
              BACKEND_HEADER_STR, ansi_colors[reset_color]);
    } else {
      fprintf(stderr, "Error [%s]: ", BACKEND_HEADER_STR);
    }
  }
  verrx(EXIT_FAILURE, fmt, argp);
}

void logger_init(void) {

  logger_enabled = is_logger_enabled();
  logger_colored = is_logger_colored();
}
