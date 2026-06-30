/*****************************************************************************\
 *                                                                           *\
 *  This file is part of the Verificarlo project,                            *\
 *  under the Apache License v2.0 with LLVM Exceptions.                      *\
 *  SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception.                 *\
 *  See https://llvm.org/LICENSE.txt for license information.                *\
 *                                                                           *\
 *  Copyright (c) 2019-2026                                                  *\
 *     Verificarlo Contributors                                              *\
 *                                                                           *\
 ****************************************************************************/

/* interflop wrapper backend for the PRISM library.
 *
 * This backend bridges the standard VFC_BACKENDS / interflop_call interface
 * to the PRISM virtual-precision API.  It does not intercept arithmetic
 * operations (PRISM does that at the IR level); it only exposes:
 *
 *   - CLI arguments --precision-binary32 and --precision-binary64 so that
 *     users can set the virtual precision via VFC_BACKENDS.
 *   - interflop_user_call handling for INTERFLOP_SET_PRECISION_BINARY32,
 *     INTERFLOP_SET_PRECISION_BINARY64 and INTERFLOP_SET_ROUNDING_MODE so that
 *     instrumented code can change precision or rounding mode at runtime via
 *     interflop_call().
 */

#include <argp.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "interflop/interflop.h"
#include "interflop/interflop_stdlib.h"
#include "interflop/iostream/logger.h"
#include "prism_api.h"

/* Default precisions match IEEE 754 hardware precision */
#define DEFAULT_PRECISION_BINARY32 24
#define DEFAULT_PRECISION_BINARY64 53

typedef struct {
  uint64_t seed;
  int32_t precision_binary32;
  int32_t precision_binary64;
  int32_t mode;
  IBool choose_seed;
} prism_context_t;

static const char backend_name[] = "interflop-prism";
static const char backend_version[] = "1.x-dev";

const char *interflop_get_backend_name(void) { return backend_name; }

const char *interflop_get_backend_version(void) { return backend_version; }

/* ---------- argp CLI parsing ---------- */

typedef enum {
  KEY_PRECISION_BINARY32 = 'f',
  KEY_PRECISION_BINARY64 = 'd',
  KEY_SEED = 's',
  KEY_MODE = 'm',
} key_args;

static const struct argp_option options[] = {
    {"precision-binary32", KEY_PRECISION_BINARY32, "N", 0,
     "Virtual precision for binary32 (default: 24)", 0},
    {"precision-binary64", KEY_PRECISION_BINARY64, "N", 0,
     "Virtual precision for binary64 (default: 53)", 0},
    {"seed", KEY_SEED, "SEED", 0,
     "Fix the random generator seed (default: random)", 0},
    {"mode", KEY_MODE, "MODE", 0,
     "select PRISM rounding mode among {sr, rn} (default: sr)", 0},
    {0, 0, 0, 0, 0, 0}};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  prism_context_t *ctx = (prism_context_t *)state->input;
  switch (key) {
  case KEY_PRECISION_BINARY32: {
    char *endptr = NULL;
    long val = strtol(arg, &endptr, 10);
    if (arg[0] == '\0' || *endptr != '\0' || val < 1 ||
        val > DEFAULT_PRECISION_BINARY32) {
      logger_error("--precision-binary32 invalid value provided, must be an "
                   "integer between 1 and %d",
                   DEFAULT_PRECISION_BINARY32);
    }
    ctx->precision_binary32 = (int32_t)val;
    break;
  }
  case KEY_PRECISION_BINARY64: {
    char *endptr = NULL;
    long val = strtol(arg, &endptr, 10);
    if (arg[0] == '\0' || *endptr != '\0' || val < 1 ||
        val > DEFAULT_PRECISION_BINARY64) {
      logger_error("--precision-binary64 invalid value provided, must be an "
                   "integer between 1 and %d",
                   DEFAULT_PRECISION_BINARY64);
    }
    ctx->precision_binary64 = (int32_t)val;
    break;
  }
  case KEY_SEED: {
    int error = 0;
    char *endptr = NULL;
    long val = interflop_strtol(arg, &endptr, &error);
    if (error != 0) {
      logger_error("--seed invalid value provided, must be an integer");
    }
    ctx->seed = (uint64_t)val;
    ctx->choose_seed = true;
    break;
  }
  case KEY_MODE: {
    if (interflop_strcasecmp(arg, "sr") == 0) {
      ctx->mode = INTERFLOP_PRISM_SR;
    } else if (interflop_strcasecmp(arg, "rn") == 0) {
      ctx->mode = INTERFLOP_PRISM_RN;
    } else {
      logger_error("--mode invalid value provided, must be one of: {sr, rn}");
    }
    break;
  }
  default:
    return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

static struct argp argp = {options, parse_opt, "", "", NULL, NULL, NULL};

/* ---------- interflop_user_call ---------- */

static void prism_user_call(void *context, interflop_call_id id, va_list ap) {
  (void)context;
  switch (id) {
  case INTERFLOP_SET_PRECISION_BINARY32: {
    int32_t prec = va_arg(ap, int);
    interflop_prism_set_default_virtual_precision_binary32(prec);
    break;
  }
  case INTERFLOP_SET_PRECISION_BINARY64: {
    int32_t prec = va_arg(ap, int);
    interflop_prism_set_default_virtual_precision_binary64(prec);
    break;
  }
  case INTERFLOP_SET_ROUNDING_MODE: {
    int32_t mode = va_arg(ap, int);
    interflop_prism_set_rounding_mode(mode);
    break;
  }
  default:
    break;
  }
}

/* ---------- interflop interface ---------- */

void interflop_pre_init(interflop_panic_t panic, File *stream, void **context) {
  interflop_set_handler("panic", panic);
  logger_init(panic, stream, backend_name);

  prism_context_t *ctx =
      (prism_context_t *)interflop_malloc(sizeof(prism_context_t));
  ctx->precision_binary32 = DEFAULT_PRECISION_BINARY32;
  ctx->precision_binary64 = DEFAULT_PRECISION_BINARY64;
  ctx->mode = INTERFLOP_PRISM_SR;
  ctx->seed = 0ULL;
  ctx->choose_seed = false;
  *context = ctx;
}

void interflop_cli(int argc, char **argv, void *context) {
  prism_context_t *ctx = (prism_context_t *)context;
  if (interflop_argp_parse != NULL) {
    interflop_argp_parse(&argp, argc, argv, 0, 0, ctx);
  } else {
    interflop_panic(
        "Interflop backend error: argp_parse not implemented\n"
        "Ensure interflop_argp_parse is provided by interflop_stdlib\n");
  }
}

struct interflop_backend_interface_t interflop_init(void *context) {
  prism_context_t *ctx = (prism_context_t *)context;

  interflop_prism_set_default_virtual_precision_binary32(
      ctx->precision_binary32);
  interflop_prism_set_default_virtual_precision_binary64(
      ctx->precision_binary64);
  interflop_prism_set_rounding_mode(ctx->mode);

  if (ctx->choose_seed) {
    interflop_prism_set_seed(ctx->seed);
  }
  logger_info("seed = %lu%s\n", interflop_prism_get_seed(),
              ctx->choose_seed ? " (fixed)" : " (random)");
  logger_info("mode = %s\n", ctx->mode == INTERFLOP_PRISM_RN ? "rn" : "sr");

  struct interflop_backend_interface_t iface = {
      .interflop_add_float = NULL,
      .interflop_sub_float = NULL,
      .interflop_mul_float = NULL,
      .interflop_div_float = NULL,
      .interflop_cmp_float = NULL,
      .interflop_add_double = NULL,
      .interflop_sub_double = NULL,
      .interflop_mul_double = NULL,
      .interflop_div_double = NULL,
      .interflop_cmp_double = NULL,
      .interflop_cast_double_to_float = NULL,
      .interflop_fma_float = NULL,
      .interflop_fma_double = NULL,
      .interflop_enter_function = NULL,
      .interflop_exit_function = NULL,
      .interflop_user_call = prism_user_call,
      .interflop_finalize = NULL,
  };
  return iface;
}
