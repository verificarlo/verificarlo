/*****************************************************************************\
 *                                                                           *\
 *  This File is part of the Verificarlo project,                            *\
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

#include <stdarg.h>
#include <stdio.h>

#include "logger.h"

#if defined(__cplusplus)
extern "C" {
#endif

static const char *backend_header = NULL;

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

/* Environment variable for specifying the verificarlo logger output File */
static const char vfc_backends_logfile[] = "VFC_BACKENDS_LOGFILE";

/* Environment variable for enabling/disabling the color */
static const char vfc_backends_colored_logger[] = "VFC_BACKENDS_COLORED_LOGGER";

static IBool logger_enabled = ITrue;
static IBool logger_colored = IFalse;
static File *logger_logfile = Null;
static File *logger_stderr = Null;

/* Returns ITrue if the logger is enabled */
IBool is_logger_enabled(void) {
  const char *is_logger_enabled_env = interflop_getenv(vfc_backends_logger);
  if (is_logger_enabled_env == Null) {
    return ITrue;
  } else if (interflop_strcasecmp(is_logger_enabled_env, "True") == 0) {
    return ITrue;
  } else {
    return IFalse;
  }
}

/* Returns ITrue if the color is enabled */
IBool is_logger_colored(void) {
  const char *is_colored_logger_env =
      interflop_getenv(vfc_backends_colored_logger);
  if (is_colored_logger_env == Null) {
    return IFalse;
  } else if (interflop_strcasecmp(is_colored_logger_env, "True") == 0) {
    return ITrue;
  } else {
    return IFalse;
  }
}

static void _interflop_err(int eval, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  interflop_vwarnx(fmt, ap);
  va_end(ap);
  interflop_exit(eval);
}

static void _interflop_verrx(int eval, const char *fmt, va_list args) {
  interflop_vwarnx(fmt, args);
  interflop_exit(eval);
}

void set_logger_logfile() {
  if (logger_logfile != Null) {
    return;
  }
  const char *logger_logfile_env = interflop_getenv(vfc_backends_logfile);
  if (logger_logfile_env == Null) {
    logger_logfile = logger_stderr;
  } else {
    /* Create log File specific to TID to avoid non-deterministic output */
    char tmp[1024];
    int r =
        interflop_sprintf(tmp, "%s.%d", logger_logfile_env, interflop_gettid());
    if (r < 0) {
      _interflop_err(EXIT_FAILURE, "Error while creating %s name\n",
                     logger_logfile_env);
    }
    int error = 0;
    logger_logfile = interflop_fopen(tmp, "a", &error);
    if (logger_logfile == Null) {
      char *msg = interflop_strerror(error);
      _interflop_err(EXIT_FAILURE, "Error [%s]: %s", backend_header, msg);
    }
  }
}

static void logger_header(File *stream, const char *lvl_name,
                          const level_color lvl_color, const IBool colored) {
  if (colored) {
    interflop_fprintf(stream, "%s%s%s [%s%s%s]: ", ansi_colors[lvl_color],
                      lvl_name, ansi_colors[reset_color],
                      ansi_colors[backend_color], backend_header,
                      ansi_colors[reset_color]);
  } else {
    interflop_fprintf(stream, "%s [%s]: ", lvl_name, backend_header);
  }
}

/* Display the info message */
void logger_info(const char *fmt, ...) {
  if (logger_enabled) {
    logger_header(logger_logfile, "Info", info_color, logger_colored);
    va_list ap;
    va_start(ap, fmt);
    interflop_vfprintf(logger_logfile, fmt, ap);
    va_end(ap);
  }
}

/* Display the warning message */
void logger_warning(const char *fmt, ...) {
  if (logger_enabled) {
    logger_header(logger_stderr, "Warning", warning_color, logger_colored);
  }
  va_list ap;
  va_start(ap, fmt);
  interflop_vwarnx(fmt, ap);
  va_end(ap);
}

/* Display the error message */
void logger_error(const char *fmt, ...) {
  if (logger_enabled) {
    logger_header(logger_stderr, "Error", error_color, logger_colored);
  }
  va_list ap;
  va_start(ap, fmt);
  _interflop_verrx(EXIT_FAILURE, fmt, ap);
  va_end(ap);
}

/* Display the info message */
void vlogger_info(const char *fmt, va_list argp) {
  if (logger_enabled) {
    logger_header(logger_logfile, "Info", info_color, logger_colored);
    interflop_vfprintf(logger_logfile, fmt, argp);
  }
}

/* Display the warning message */
void vlogger_warning(const char *fmt, va_list argp) {
  if (logger_enabled) {
    logger_header(logger_stderr, "Warning", warning_color, logger_colored);
  }
  interflop_vwarnx(fmt, argp);
}

/* Display the error message */
void vlogger_error(const char *fmt, va_list argp) {
  if (logger_enabled) {
    logger_header(logger_stderr, "Error", error_color, logger_colored);
  }
  _interflop_verrx(EXIT_FAILURE, fmt, argp);
}

static void _logger_check_stdlib(void) {
  INTERFLOP_CHECK_IMPL(exit);
  INTERFLOP_CHECK_IMPL(fopen);
  INTERFLOP_CHECK_IMPL(fprintf);
  INTERFLOP_CHECK_IMPL(getenv);
  INTERFLOP_CHECK_IMPL(gettid);
  INTERFLOP_CHECK_IMPL(sprintf);
  INTERFLOP_CHECK_IMPL(strcasecmp);
  INTERFLOP_CHECK_IMPL(strerror);
  INTERFLOP_CHECK_IMPL(vfprintf);
  INTERFLOP_CHECK_IMPL(vwarnx);
}

void logger_init(interflop_panic_t panic, File *stream,
                 const char *backend_header_name) {
  backend_header = backend_header_name;
  logger_stderr = stream;
  interflop_set_handler("panic", (void *)panic);
  _logger_check_stdlib();

  logger_enabled = is_logger_enabled();
  logger_colored = is_logger_colored();
  set_logger_logfile();
}

#if defined(__cplusplus)
}
#endif