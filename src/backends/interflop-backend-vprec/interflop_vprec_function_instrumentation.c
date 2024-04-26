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

#include <argp.h>

#include "common/vprec_tools.h"
#include "interflop/hashmap/vfc_hashmap.h"
#include "interflop/interflop.h"
#include "interflop/interflop_stdlib.h"
#include "interflop_vprec.h"
#include "interflop_vprec_function_instrumentation.h"

/******************** VPREC FUNCTIONS INSTRUMENTATION (VFI) **************
 * The following set of functions is used to apply vprec on instrumented
 * functions. For that we need a hashmap to stock data and reading and
 * writing functions to get and save them. Enter and exit functions are
 * called before and after the instrumented function and allow us to set
 * the desired precision or to round arguments, depending on the mode.
 *************************************************************************/

/* instrumentation modes' names */
static const char *VPREC_INST_MODE_STR[] = {[vprecinst_arg] = "arguments",
                                            [vprecinst_op] = "operations",
                                            [vprecinst_all] = "all",
                                            [vprecinst_none] = "none"};

static const char key_instrument_str[] = "instrument";
static const char key_input_file_str[] = "prec-input-file";
static const char key_output_file_str[] = "prec-output-file";
static const char key_log_file_str[] = "prec-log-file";

#define STRING_BUFF 256

#define INIT_STRING(A, N)                                                      \
  for (int i = 0; i < N; i++)                                                  \
    A[i] = (char *)interflop_malloc(sizeof(char) * STRING_BUFF);

#define FREE_STRING(A, N)                                                      \
  for (int i = 0; i < N; i++)                                                  \
    if (A[i])                                                                  \
      interflop_free(A[i]);

const int elt_to_read_header = 12;
const int elt_to_read_inputs = 7;
const int elt_to_read_outputs = 7;

char *tokens_header[12];
char *tokens_inputs[7];
char *tokens_outputs[7];

static File *_vprec_log_file = Null;

/* Setter functions for variables */

void _set_vprec_input_file(const char *input_file, void *context) {
  vprec_context_t *ctx = (vprec_context_t *)context;
  ctx->vfi->vprec_input_file = input_file;
}

void _set_vprec_output_file(const char *output_file, void *context) {
  vprec_context_t *ctx = (vprec_context_t *)context;
  ctx->vfi->vprec_output_file = output_file;
}

void _set_vprec_log_file(const char *log_file, void *context) {
  vprec_context_t *ctx = (vprec_context_t *)context;
  ctx->vfi->vprec_log_file = log_file;
}

void _set_vprec_inst_mode(vprec_inst_mode mode, void *context) {
  vprec_context_t *ctx = (vprec_context_t *)context;
  if (mode >= _vprecinst_end_) {
    logger_error("invalid instrumentation mode provided, must be one of:"
                 "{arguments, operations, all, none}.");
  } else {
    ctx->vfi->vprec_inst_mode = mode;
  }
}

/* Argument parser functions */

void _parse_key_instrument(char *arg, vprec_context_t *ctx) {
  /* instrumentation mode */
  if (interflop_strcasecmp(VPREC_INST_MODE_STR[vprecinst_arg], arg) == 0) {
    _set_vprec_inst_mode(vprecinst_arg, ctx);
  } else if (interflop_strcasecmp(VPREC_INST_MODE_STR[vprecinst_op], arg) ==
             0) {
    _set_vprec_inst_mode(vprecinst_op, ctx);
  } else if (interflop_strcasecmp(VPREC_INST_MODE_STR[vprecinst_all], arg) ==
             0) {
    _set_vprec_inst_mode(vprecinst_all, ctx);
  } else if (interflop_strcasecmp(VPREC_INST_MODE_STR[vprecinst_none], arg) ==
             0) {
    _set_vprec_inst_mode(vprecinst_none, ctx);
  } else {
    logger_error("--%s invalid value provided, must be one of: "
                 "{arguments, operations, all}.",
                 key_instrument_str);
  }
}

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  vprec_context_t *ctx = (vprec_context_t *)state->input;
  switch (key) {
  case KEY_INPUT_FILE:
    /* input file */
    _set_vprec_input_file(arg, ctx);
    break;
  case KEY_OUTPUT_FILE:
    /* output file */
    _set_vprec_output_file(arg, ctx);
    break;
  case KEY_LOG_FILE:
    /* log file */
    _set_vprec_log_file(arg, ctx);
    break;
  case KEY_INSTRUMENT:
    _parse_key_instrument(arg, ctx);
    break;

  default:
    return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

static struct argp_option options[] = {
    {key_input_file_str, KEY_INPUT_FILE, "INPUT", 0,
     "input file with the precision configuration to use", 0},
    {key_output_file_str, KEY_OUTPUT_FILE, "OUTPUT", 0,
     "output file where the precision profile is written", 0},
    {key_log_file_str, KEY_LOG_FILE, "LOG", 0,
     "log file where input/output informations are written", 0},
    {key_instrument_str, KEY_INSTRUMENT, "INSTRUMENTATION", 0,
     "select VPREC instrumentation mode among {arguments, operations, full}",
     0},
    {0}};

struct argp vfi_argp = {options, parse_opt, "", "", NULL, NULL, NULL};

void _vfi_print_information_header(void *context) {
  vprec_context_t *ctx = (vprec_context_t *)context;
  logger_info("\t%s = %s\n", key_instrument_str,
              VPREC_INST_MODE_STR[ctx->vfi->vprec_inst_mode]);
  logger_info("\t%s = %s\n", key_input_file_str, ctx->vfi->vprec_input_file);
  logger_info("\t%s = %s\n", key_output_file_str, ctx->vfi->vprec_output_file);
  logger_info("\t%s = %s\n", key_log_file_str, ctx->vfi->vprec_log_file);
}

/* Core functions */

// Write the hashmap in the given file
void _vfi_write_hasmap(FILE *fout, vprec_context_t *ctx) {
  for (size_t ii = 0; ii < ctx->vfi->map->capacity; ii++) {
    if (get_value_at(ctx->vfi->map->items, ii) != 0 &&
        get_value_at(ctx->vfi->map->items, ii) != 0) {
      _vfi_t *function = (_vfi_t *)get_value_at(ctx->vfi->map->items, ii);

      interflop_fprintf(
          fout, "%s\t%hd\t%hd\t%zu\t%zu\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",
          function->id, function->isLibraryFunction,
          function->isIntrinsicFunction, function->useFloat,
          function->useDouble, function->OpsPrec64, function->OpsRange64,
          function->OpsPrec32, function->OpsRange32, function->nb_input_args,
          function->nb_output_args, function->n_calls);
      for (int i = 0; i < function->nb_input_args; i++) {
        interflop_fprintf(fout, "input:\t%s\t%hd\t%d\t%d\t%d\t%d\n",
                          function->input_args[i].arg_id,
                          function->input_args[i].data_type,
                          function->input_args[i].mantissa_length,
                          function->input_args[i].exponent_length,
                          function->input_args[i].min_range,
                          function->input_args[i].max_range);
      }
      for (int i = 0; i < function->nb_output_args; i++) {
        interflop_fprintf(fout, "output:\t%s\t%hd\t%d\t%d\t%d\t%d\n",
                          function->output_args[i].arg_id,
                          function->output_args[i].data_type,
                          function->output_args[i].mantissa_length,
                          function->output_args[i].exponent_length,
                          function->output_args[i].min_range,
                          function->output_args[i].max_range);
      }
    }
  }
}

/* Helper function scanning an integer */
/* return the integer upon success */
/* otherwise call logger_error  */
long _vfi_scan_int(char *token, const char *field) {
  int error = 0;
  char *endptr;
  long res = interflop_strtol(token, &endptr, &error);
  if (error != 0) {
    logger_error("Error while reading hashmap config file (field: %s)\n",
                 field);
  }
  return res;
}

int _vfi_scan_line(FILE *fi, char **tokens) {
  const int line_max_size = 2048;
  char line[2048];
  interflop_fgets(line, line_max_size, fi);
  char *tabptr;
  char *token = interflop_strtok_r(line, "\t", &tabptr);
  int nb_token = 0;
  while (token) {
    interflop_strcpy(tokens[nb_token], token);
    nb_token++;
    token = interflop_strtok_r(NULL, "\t", &tabptr);
  }
  return nb_token;
}

int _vfi_scan_header(FILE *fi, _vfi_t *function_ptr) {
  int nb_token = _vfi_scan_line(fi, tokens_header);
  if (nb_token != elt_to_read_header) {
    return nb_token;
  }

  interflop_strcpy(function_ptr->id, tokens_header[0]);
  function_ptr->isLibraryFunction =
      _vfi_scan_int(tokens_header[1], "isLibraryFunction");
  function_ptr->isIntrinsicFunction =
      _vfi_scan_int(tokens_header[2], "isIntrinsicFunction");
  function_ptr->useFloat = _vfi_scan_int(tokens_header[3], "useFloat");
  function_ptr->useDouble = _vfi_scan_int(tokens_header[4], "useDouble");
  function_ptr->OpsPrec64 = _vfi_scan_int(tokens_header[5], "OpsPrec64");
  function_ptr->OpsRange64 = _vfi_scan_int(tokens_header[6], "OpsRange64");
  function_ptr->OpsPrec32 = _vfi_scan_int(tokens_header[7], "OpsPrec32");
  function_ptr->OpsRange32 = _vfi_scan_int(tokens_header[8], "OpsRange32");
  function_ptr->nb_input_args =
      _vfi_scan_int(tokens_header[9], "nb_input_args");
  function_ptr->nb_output_args =
      _vfi_scan_int(tokens_header[10], "nb_output_args");
  function_ptr->n_calls = _vfi_scan_int(tokens_header[11], "n_calls");

  return nb_token;
}

int _vfi_scan_input(FILE *fi, _vfi_t *function_ptr, int arg_pos) {
  int nb_token = _vfi_scan_line(fi, tokens_inputs);
  _vfi_argument_data_t *arg_data = &function_ptr->input_args[arg_pos];

  // tokens[0] == "input:"
  interflop_strcpy(arg_data->arg_id, tokens_inputs[1]);
  arg_data->data_type = _vfi_scan_int(tokens_inputs[2], "data_type");
  arg_data->mantissa_length =
      _vfi_scan_int(tokens_inputs[3], "mantissa_length");
  arg_data->exponent_length =
      _vfi_scan_int(tokens_inputs[4], "exponent_length");
  arg_data->min_range = _vfi_scan_int(tokens_inputs[5], "min_range");
  arg_data->max_range = _vfi_scan_int(tokens_inputs[6], "max_range");

  return nb_token;
}

int _vfi_scan_output(FILE *fi, _vfi_t *function_ptr, int arg_pos) {
  int nb_token = _vfi_scan_line(fi, tokens_outputs);
  _vfi_argument_data_t *arg_data = &function_ptr->output_args[arg_pos];

  // tokens[0] == "output:"
  interflop_strcpy(arg_data->arg_id, tokens_outputs[1]);
  arg_data->data_type = _vfi_scan_int(tokens_outputs[2], "data_type");
  arg_data->mantissa_length =
      _vfi_scan_int(tokens_outputs[3], "mantissa_length");
  arg_data->exponent_length =
      _vfi_scan_int(tokens_outputs[4], "exponent_length");
  arg_data->min_range = _vfi_scan_int(tokens_outputs[5], "min_range");
  arg_data->max_range = _vfi_scan_int(tokens_outputs[6], "max_range");

  return nb_token;
}

// Read and initialize the hashmap from the given file
void _vfi_read_hasmap(FILE *fin, vprec_context_t *ctx) {
  _vfi_t function;

  while (_vfi_scan_header(fin, &function) == 12) {
    // allocate space for input arguments
    function.input_args = (_vfi_argument_data_t *)interflop_calloc(
        function.nb_input_args, sizeof(_vfi_argument_data_t));

    // allocate space for output arguments
    function.output_args = (_vfi_argument_data_t *)interflop_calloc(
        function.nb_output_args, sizeof(_vfi_argument_data_t));

    const int elt_to_read = 7;
    // get input arguments precision
    for (int i = 0; i < function.nb_input_args; i++) {
      if (_vfi_scan_input(fin, &function, i) != elt_to_read) {
        logger_error("Can't read input arguments of %s\n", function.id);
      }
    }

    // get output arguments precision
    for (int i = 0; i < function.nb_output_args; i++) {
      if (_vfi_scan_output(fin, &function, i) != elt_to_read) {
        logger_error("Can't read output arguments of %s\n", function.id);
      }
    }

    // insert in the hashmap
    _vfi_t *address = (_vfi_t *)interflop_malloc(sizeof(_vfi_t));
    (*address) = function;
    vfc_hashmap_insert(ctx->vfi->map, vfc_hashmap_str_function(function.id),
                       address);
  }
}

// Print str in vprec_log_file with the correct offset
#define _vfi_print_log(ctx, _vprec_str, ...)                                   \
  ({                                                                           \
    if (_vprec_log_file != NULL) {                                             \
      for (size_t _vprec_d = 0; _vprec_d < ctx->vfi->vprec_log_depth;          \
           _vprec_d++)                                                         \
        interflop_fprintf(_vprec_log_file, "\t");                              \
      interflop_fprintf(_vprec_log_file, _vprec_str, ##__VA_ARGS__);           \
    }                                                                          \
  })

/* allocate the context */
void _vfi_alloc_context(void *context) {
  vprec_context_t *ctx = (vprec_context_t *)context;
  ctx->vfi = (t_context_vfi *)interflop_malloc(sizeof(t_context_vfi));
}

/* initialize the context */
void _vfi_init_context(void *context) {
  vprec_context_t *ctx = (vprec_context_t *)context;
  ctx->vfi->map = NULL;
  ctx->vfi->vprec_input_file = NULL;
  ctx->vfi->vprec_output_file = NULL;
  ctx->vfi->vprec_log_file = NULL;
  ctx->vfi->vprec_inst_mode = VPREC_INST_MODE_DEFAULT;
  ctx->vfi->vprec_log_depth = 0;
}

/* initialize the variables to run vprec function instrumentation */
void _vfi_init(void *context) {
  INIT_STRING(tokens_header, elt_to_read_header);
  INIT_STRING(tokens_inputs, elt_to_read_inputs);
  INIT_STRING(tokens_outputs, elt_to_read_outputs);

  vprec_context_t *ctx = (vprec_context_t *)context;
  /* Initialize the vprec_function_map */

  ctx->vfi->map = vfc_hashmap_create();
  /* read the hashmap */
  if (ctx->vfi->vprec_input_file != NULL) {
    int error = 0;
    File *f = interflop_fopen(ctx->vfi->vprec_input_file, "r", &error);
    if (f != NULL) {
      _vfi_read_hasmap(f, ctx);
      interflop_fclose(f);
    } else {
      logger_error("Input file can't be found: %s", interflop_strerror(error));
    }
  }

  if (ctx->vfi->vprec_log_file != NULL) {
    int error = 0;
    File *f = interflop_fopen(ctx->vfi->vprec_log_file, "w", &error);
    if (f != NULL) {
      _vprec_log_file = f;
    } else {
      logger_error("Error while opening %s: %s", ctx->vfi->vprec_log_file,
                   interflop_strerror(error));
    }
  }
}

/* free objects and close files */
void _vfi_finalize(void *context) {
  vprec_context_t *ctx = (vprec_context_t *)context;

  /* save the hashmap */
  if (ctx->vfi->vprec_output_file != NULL) {
    int error = 0;
    File *f = interflop_fopen(ctx->vfi->vprec_output_file, "w", &error);
    if (f != NULL) {
      _vfi_write_hasmap(f, ctx);
      interflop_fclose(f);
    } else {
      logger_error("Output file can't be written: %s",
                   interflop_strerror(error));
    }
  }

  /* close log file */
  if (_vprec_log_file != NULL) {
    interflop_fclose(_vprec_log_file);
  }

  /* free vprec_function_map */
  vfc_hashmap_free(ctx->vfi->map);

  /* destroy vprec_function_map */
  vfc_hashmap_destroy(ctx->vfi->map);

  FREE_STRING(tokens_header, elt_to_read_header);
  FREE_STRING(tokens_inputs, elt_to_read_inputs);
  FREE_STRING(tokens_outputs, elt_to_read_outputs);
}

void _init_function_inst_arg(_vfi_argument_data_t *arg, char *arg_id,
                             enum FTYPES type) {
  arg->data_type = type;
  interflop_strcpy(arg->arg_id, arg_id);
  arg->min_range = INT_MAX;
  arg->max_range = INT_MIN;
  arg->exponent_length = (type == FDOUBLE || type == FDOUBLE_PTR)
                             ? VPREC_RANGE_BINARY64_DEFAULT
                             : VPREC_RANGE_BINARY32_DEFAULT;
  arg->mantissa_length = (type == FDOUBLE || type == FDOUBLE_PTR)
                             ? VPREC_PRECISION_BINARY64_DEFAULT
                             : VPREC_PRECISION_BINARY32_DEFAULT;
}

void _update_range_bounds(void *raw_value, _vfi_argument_data_t *arg,
                          int new_flag, enum FTYPES type) {

  int isnan = 0, isinf = 0, floor = 0, ceil = 0;
  if (type == FFLOAT || type == FFLOAT_PTR) {
    float *value = (float *)raw_value;
    isnan = interflop_isnan(*value);
    isinf = interflop_isinf(*value);
    floor = interflop_floor(*value);
    ceil = interflop_ceil(*value);
  } else if (type == FDOUBLE || type == FDOUBLE_PTR) {
    double *value = (double *)raw_value;
    isnan = interflop_isnan(*value);
    isinf = interflop_isinf(*value);
    floor = interflop_floor(*value);
    ceil = interflop_ceil(*value);
  } else {
    logger_error("Invalid type\n");
  }

  const int min_range = arg->min_range;
  const int max_range = arg->max_range;
  if (!(isnan || isinf)) {
    arg->min_range = (floor < min_range || new_flag) ? floor : min_range;
    arg->max_range = (ceil > max_range || new_flag) ? ceil : max_range;
  }
}

void _vprec_round_binary(char is_input, void *raw_value, int exponent_length,
                         int mantissa_length, int new_flag, int mode_flag,
                         enum FTYPES type, vprec_context_t *context) {
  if ((!new_flag) && mode_flag) {
    if (type == FFLOAT || type == FFLOAT_PTR) {
      float *value = (float *)raw_value;
      *value = _vprec_round_binary32(*value, is_input, context, exponent_length,
                                     mantissa_length);
    } else if (type == FDOUBLE || type == FDOUBLE_PTR) {
      double *value = (double *)raw_value;
      *value = _vprec_round_binary64(*value, is_input, context, exponent_length,
                                     mantissa_length);
    } else {
      logger_error("Invalid type\n");
    }
  }
}

void _vprec_round_binary_enter(void *raw_value, int exponent_length,
                               int mantissa_length, int new_flag, int mode_flag,
                               enum FTYPES type, vprec_context_t *context) {
  _vprec_round_binary(1, raw_value, exponent_length, mantissa_length, new_flag,
                      mode_flag, type, context);
}

void _vprec_round_binary_exit(void *raw_value, int exponent_length,
                              int mantissa_length, int new_flag, int mode_flag,
                              enum FTYPES type, vprec_context_t *context) {
  _vprec_round_binary(0, raw_value, exponent_length, mantissa_length, new_flag,
                      mode_flag, type, context);
}

void _vfi_print_log_helper(const char *header, void *raw_value,
                           _vfi_t *function_inst, char *arg_id, int j,
                           enum FTYPES type, vprec_context_t *ctx) {

  if (type == FFLOAT) {
    float *value = (float *)raw_value;
    _vfi_print_log(ctx, " - %s\t%s\tfloat\t%s\t%la\t->\t", header,
                   function_inst->id, arg_id, *value);
  } else if (type == FDOUBLE) {
    double *value = (double *)raw_value;
    _vfi_print_log(ctx, " - %s\t%s\tdouble\t%s\t%la\t->\t", header,
                   function_inst->id, arg_id, *value);
  } else if (type == FFLOAT_PTR) {
    float *value = (float *)raw_value;
    if (value == NULL) {
      _vfi_print_log(ctx, " - %s\t%s[%u]\tfloat_ptr\t%s\tNULL\t->\t", header,
                     function_inst->id, j, arg_id);
    } else {
      _vfi_print_log(ctx, " - %s\t%s[%u]\tfloat_ptr\t%s\t%a\t->\t", header,
                     function_inst->id, j, arg_id, *value);
    }
  } else if (type == FDOUBLE_PTR) {
    double *value = (double *)raw_value;
    if (value == NULL) {
      _vfi_print_log(ctx, " - %s\t%s[%u]\tdouble_ptr\t%s\tNULL\t->\t", header,
                     function_inst->id, j, arg_id);
    } else {
      _vfi_print_log(ctx, " - %s\t%s[%u]\tdouble_ptr\t%s\t%a\t->\t", header,
                     function_inst->id, j, arg_id, *value);
    }
  }
}

void _vfi_print_log_enter(void *raw_value, _vfi_t *function_inst, char *arg_id,
                          int j, enum FTYPES type, vprec_context_t *ctx) {
  _vfi_print_log_helper("input", raw_value, function_inst, arg_id, j, type,
                        ctx);
}

void _vfi_print_log_exit(void *raw_value, _vfi_t *function_inst, char *arg_id,
                         int j, enum FTYPES type, vprec_context_t *ctx) {
  _vfi_print_log_helper("output", raw_value, function_inst, arg_id, j, type,
                        ctx);
}

// vprec function instrumentation
// Set precision for internal operations and round input arguments for a given
// function call
void _vfi_enter_function(interflop_function_stack_t *stack, void *context,
                         int nb_args, va_list ap) {
  vprec_context_t *ctx = (vprec_context_t *)context;

  interflop_function_info_t *function_info = stack->array[stack->top];

  if (function_info == NULL)
    logger_error("Call stack error\n");

  _vfi_t *function_inst = vfc_hashmap_get(
      ctx->vfi->map, vfc_hashmap_str_function(function_info->id));

  // if the function is not in the hashtable
  if (function_inst == NULL) {
    function_inst = interflop_malloc(sizeof(_vfi_t));

    // initialize the structure
    interflop_strcpy(function_inst->id, function_info->id);
    function_inst->isLibraryFunction = function_info->isLibraryFunction;
    function_inst->isIntrinsicFunction = function_info->isIntrinsicFunction;
    function_inst->useFloat = function_info->useFloat;
    function_inst->useDouble = function_info->useDouble;
    function_inst->OpsRange64 = VPREC_RANGE_BINARY64_DEFAULT;
    function_inst->OpsPrec64 = VPREC_PRECISION_BINARY64_DEFAULT;
    function_inst->OpsRange32 = VPREC_RANGE_BINARY32_DEFAULT;
    function_inst->OpsPrec32 = VPREC_PRECISION_BINARY32_DEFAULT;
    function_inst->nb_input_args = 0;
    function_inst->input_args = NULL;
    function_inst->nb_output_args = 0;
    function_inst->output_args = NULL;
    function_inst->n_calls = 0;

    // insert the function in the hashmap
    vfc_hashmap_insert(ctx->vfi->map,
                       vfc_hashmap_str_function(function_info->id),
                       function_inst);
  }

  // increment the number of calls
  function_inst->n_calls++;

  // set internal operations precision with custom values depending on the mode
  if (!function_info->isLibraryFunction &&
      !function_info->isIntrinsicFunction &&
      ctx->vfi->vprec_inst_mode != vprecinst_arg &&
      ctx->vfi->vprec_inst_mode != vprecinst_none) {
    _set_vprec_precision_binary64(function_inst->OpsPrec64, ctx);
    _set_vprec_range_binary64(function_inst->OpsRange64, ctx);
    _set_vprec_precision_binary32(function_inst->OpsPrec32, ctx);
    _set_vprec_range_binary32(function_inst->OpsRange32, ctx);
  }

  // treatment of arguments
  int new_flag = (function_inst->input_args == NULL && nb_args > 0);

  // print function info in log
  _vfi_print_log(ctx, "\n");
  _vfi_print_log(ctx, "enter in %s\t%d\t%d\t%d\t%d\n", function_inst->id,
                 function_inst->OpsPrec64, function_inst->OpsRange64,
                 function_inst->OpsPrec32, function_inst->OpsRange32);

  // allocate memory for arguments
  if (new_flag) {
    function_inst->input_args =
        interflop_calloc(nb_args, sizeof(_vfi_argument_data_t));
    function_inst->nb_input_args = nb_args;
  }

  // boolean which indicates if arguments should be rounded or not depending on
  // modes
  int mode_flag =
      (((ctx->mode == vprecmode_full) || (ctx->mode == vprecmode_ib)) &&
       ((ctx->vfi->vprec_inst_mode == vprecinst_all) ||
        (ctx->vfi->vprec_inst_mode == vprecinst_arg)) &&
       ctx->vfi->vprec_inst_mode != vprecinst_none);

  for (int i = 0; i < nb_args; i++) {
    // get argument type, id and size
    int type = va_arg(ap, int);
    char *arg_id = va_arg(ap, char *);
    unsigned int size = va_arg(ap, unsigned int);
    void *raw_value = va_arg(ap, void *);

    _vfi_argument_data_t *arg = &function_inst->input_args[i];
    const int exponent_length = arg->exponent_length;
    const int mantissa_length = arg->mantissa_length;

    if (new_flag) {
      _init_function_inst_arg(arg, arg_id, type);
    }

    if (type == FDOUBLE) {
      double *value = (double *)raw_value;
      _vfi_print_log_enter(value, function_inst, arg_id, 0, type, ctx);
      _vprec_round_binary_enter(raw_value, exponent_length, mantissa_length,
                                new_flag, mode_flag, type, context);
      _update_range_bounds(raw_value, arg, new_flag, type);
      _vfi_print_log(ctx, "%la\t(%d, %d)\n", *value, mantissa_length,
                     exponent_length);

    } else if (type == FFLOAT) {
      float *value = (float *)raw_value;
      _vfi_print_log_enter(raw_value, function_inst, arg_id, 0, type, ctx);
      _vprec_round_binary_enter(raw_value, exponent_length, mantissa_length,
                                new_flag, mode_flag, type, context);
      _update_range_bounds(raw_value, arg, new_flag, type);
      _vfi_print_log(ctx, "%a\t(%d, %d)\n", *value, mantissa_length,
                     exponent_length);

    } else if (type == FDOUBLE_PTR) {
      double *value = (double *)raw_value;
      _vfi_print_log_enter(value, function_inst, arg_id, 0, type, ctx);
      if (value == NULL) {
        continue;
      }

      for (unsigned int j = 0; j < size; j++, value++) {
        _vfi_print_log_enter(value, function_inst, arg_id, j, type, ctx);
        if (value == NULL) {
          continue;
        }
        _vprec_round_binary_enter(raw_value, exponent_length, mantissa_length,
                                  new_flag, mode_flag, type, context);
        _update_range_bounds(raw_value, arg, new_flag, type);
        _vfi_print_log(ctx, "%la\t(%d, %d)\n", *value, mantissa_length,
                       exponent_length);
      }
    } else if (type == FFLOAT_PTR) {
      float *value = (float *)raw_value;
      _vfi_print_log_enter(value, function_inst, arg_id, 0, type, ctx);
      if (value == NULL) {
        continue;
      }

      for (unsigned int j = 0; j < size; j++, value++) {
        _vfi_print_log_enter(value, function_inst, arg_id, j, type, ctx);
        if (value == NULL) {
          continue;
        }
        _vprec_round_binary_enter(raw_value, exponent_length, mantissa_length,
                                  new_flag, mode_flag, type, context);
        _update_range_bounds(raw_value, arg, new_flag, type);
        _vfi_print_log(ctx, "%a\t(%d, %d)\n", *value, mantissa_length,
                       exponent_length);
      }
    }
  }

  // increment depth
  ctx->vfi->vprec_log_depth++;
}

// vprec function instrumentation
// Set precision for internal operations and round output arguments for a given
// function call
void _vfi_exit_function(interflop_function_stack_t *stack, void *context,
                        int nb_args, va_list ap) {
  vprec_context_t *ctx = (vprec_context_t *)context;

  interflop_function_info_t *function_info = stack->array[stack->top];

  // decrement depth
  ctx->vfi->vprec_log_depth--;

  if (function_info == NULL)
    logger_error("Call stack error \n");

  _vfi_t *function_inst = vfc_hashmap_get(
      ctx->vfi->map, vfc_hashmap_str_function(function_info->id));

  // set internal operations precision with parent function values
  if (stack->array[stack->top + 1] != NULL) {
    interflop_function_info_t *parent_info = stack->array[stack->top + 1];

    if (!parent_info->isLibraryFunction && !parent_info->isIntrinsicFunction &&
        ctx->vfi->vprec_inst_mode != vprecinst_arg &&
        ctx->vfi->vprec_inst_mode != vprecinst_none) {

      _vfi_t *function_parent = vfc_hashmap_get(
          ctx->vfi->map, vfc_hashmap_str_function(parent_info->id));

      if (function_parent != NULL) {
        _set_vprec_precision_binary64(function_parent->OpsPrec64, ctx);
        _set_vprec_range_binary64(function_parent->OpsRange64, ctx);
        _set_vprec_precision_binary32(function_parent->OpsPrec32, ctx);
        _set_vprec_range_binary32(function_parent->OpsRange32, ctx);
      }
    }
  }

  // treatment of arguments
  int new_flag = (function_inst->output_args == NULL && nb_args > 0);

  // print function info in log
  _vfi_print_log(ctx, "exit of %s\t%d\t%d\t%d\t%d\n", function_inst->id,
                 function_inst->OpsPrec64, function_inst->OpsRange64,
                 function_inst->OpsPrec32, function_inst->OpsRange32);

  // allocate memory for arguments
  if (new_flag) {
    function_inst->output_args =
        interflop_calloc(nb_args, sizeof(_vfi_argument_data_t));
    function_inst->nb_output_args = nb_args;
  }

  // boolean which indicates if arguments should be rounded or not depending on
  // modes
  int mode_flag =
      (((ctx->mode == vprecmode_full) || (ctx->mode == vprecmode_ob)) &&
       (ctx->vfi->vprec_inst_mode == vprecinst_all ||
        ctx->vfi->vprec_inst_mode == vprecinst_arg) &&
       ctx->vfi->vprec_inst_mode != vprecinst_none);

  for (int i = 0; i < nb_args; i++) {
    int type = va_arg(ap, int);
    char *arg_id = va_arg(ap, char *);
    unsigned int size = va_arg(ap, unsigned int);
    void *raw_value = va_arg(ap, void *);

    _vfi_argument_data_t *arg = &function_inst->output_args[i];
    const int exponent_length = arg->exponent_length;
    const int mantissa_length = arg->mantissa_length;

    if (new_flag) {
      // initialize arguments data
      _init_function_inst_arg(arg, arg_id, type);
    }

    if (type == FDOUBLE) {
      double *value = (double *)raw_value;
      _vfi_print_log_exit(value, function_inst, arg_id, 0, type, ctx);
      _vprec_round_binary_exit(raw_value, exponent_length, mantissa_length,
                               new_flag, mode_flag, type, context);
      _update_range_bounds(raw_value, arg, new_flag, type);

      _vfi_print_log(ctx, "%la\t(%d,%d)\n", *value, mantissa_length,
                     exponent_length);
    } else if (type == FFLOAT) {
      float *value = (float *)raw_value;
      _vfi_print_log_exit(value, function_inst, arg_id, 0, type, ctx);
      _vprec_round_binary_exit(raw_value, exponent_length, mantissa_length,
                               new_flag, mode_flag, type, context);
      _update_range_bounds(raw_value, arg, new_flag, type);

      _vfi_print_log(ctx, "%la\t(%d,%d)\n", *value, mantissa_length,
                     exponent_length);
    } else if (type == FDOUBLE_PTR) {

      double *value = (double *)raw_value;
      _vfi_print_log_exit(value, function_inst, arg_id, 0, type, ctx);
      if (value == NULL) {
        continue;
      }

      for (unsigned int j = 0; j < size; j++, value++) {
        _vfi_print_log_exit(value, function_inst, arg_id, j, type, ctx);
        if (value == NULL) {
          continue;
        }

        _vfi_print_log_exit(value, function_inst, arg_id, j, type, ctx);
        _vprec_round_binary_exit(raw_value, exponent_length, mantissa_length,
                                 new_flag, mode_flag, type, context);
        _update_range_bounds(raw_value, arg, new_flag, type);

        _vfi_print_log(ctx, "%la\t(%d,%d)\n", *value, mantissa_length,
                       exponent_length);
      }

    } else if (type == FFLOAT_PTR) {
      float *value = (float *)raw_value;
      _vfi_print_log_exit(value, function_inst, arg_id, 0, type, ctx);
      if (value == NULL) {
        continue;
      }

      for (unsigned int j = 0; j < size; j++, value++) {
        _vfi_print_log_exit(value, function_inst, arg_id, j, type, ctx);
        if (value == NULL) {
          continue;
        }

        _vfi_print_log_exit(value, function_inst, arg_id, j, type, ctx);
        _vprec_round_binary_exit(raw_value, exponent_length, mantissa_length,
                                 new_flag, mode_flag, type, context);
        _update_range_bounds(raw_value, arg, new_flag, type);

        _vfi_print_log(ctx, "%la\t(%d,%d)\n", *value, mantissa_length,
                       exponent_length);
      }
    }
  }
  _vfi_print_log(ctx, "\n");
}