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

#ifndef __INTERFLOP_VPREC_FUNCTION_INSTRUMENTATION_H__
#define __INTERFLOP_VPREC_FUNCTION_INSTRUMENTATION_H__

#include "interflop/hashmap/vfc_hashmap.h"
#include "interflop/interflop.h"
#include "interflop/interflop_stdlib.h"

#define ARG_ID_MAX_LENGTH 128
#define FUNCTION_ID_MAX_LENGTH 512

/* define instrumentation modes */
typedef enum {
  vprecinst_arg,
  vprecinst_op,
  vprecinst_all,
  vprecinst_none,
  _vprecinst_end_
} vprec_inst_mode;

// Metadata of arguments
typedef struct _vprec_argument_data {
  // Identifier of the argument
  char arg_id[ARG_ID_MAX_LENGTH];
  // Data type of the argument 0 is float and 1 is double
  short data_type;
  // Minimum rounded value of the argument
  int min_range;
  // Maximum rounded value of the argument
  int max_range;
  // Exponent length of the argument
  int exponent_length;
  // Mantissa length of the argument
  int mantissa_length;
} _vfi_argument_data_t;

// Metadata of function calls
typedef struct _vprec_function_instrumentation {
  // Id of the function
  char id[FUNCTION_ID_MAX_LENGTH];
  // Indicate if the function is from library
  short isLibraryFunction;
  // Indicate if the function is intrinsic
  short isIntrinsicFunction;
  // Counter of Floating Point instruction
  ISize_t useFloat;
  // Counter of Floating Point instruction
  ISize_t useDouble;
  // Internal Operations Range64
  int OpsRange64;
  // Internal Operations Prec64
  int OpsPrec64;
  // Internal Operations Range32
  int OpsRange32;
  // Internal Operations Prec32
  int OpsPrec32;
  // Number of floating point input arguments
  int nb_input_args;
  // Array of data on input arguments
  _vfi_argument_data_t *input_args;
  // Number of floating point output arguments
  int nb_output_args;
  // Array of data on output arguments
  _vfi_argument_data_t *output_args;
  // Number of call for this call site
  int n_calls;
} _vfi_t;

/* default instrumentation mode */
#define VPREC_INST_MODE_DEFAULT vprecinst_none

typedef struct {
  /* instrumentation variables */
  vfc_hashmap_t map;
  const char *vprec_input_file;
  const char *vprec_output_file;
  const char *vprec_log_file;
  vprec_inst_mode vprec_inst_mode;
  ISize_t vprec_log_depth;
} t_context_vfi;

/* Setter functions for contextual variables */
void _set_vprec_input_file(const char *input_file, void *context);
void _set_vprec_output_file(const char *output_file, void *context);
void _set_vprec_log_file(const char *log_file, void *context);
void _set_vprec_inst_mode(vprec_inst_mode mode, void *context);
void _vfi_print_information_header(void *context);

/* Vprec Function Instrumentation initializer */
void _vfi_init(void *context);

/* Vprec Function Instrumentation context initializer */
void _vfi_init_context(void *context);

/* Vprec Function Instrumentation context initializer */
void _vfi_alloc_context(void *context);

/* Vprec Function Instrumentation finalizer */
void _vfi_finalize(void *context);

/* Vprec Function Instrumentation enter function */
void _vfi_enter_function(interflop_function_stack_t *stack, void *context,
                         int nb_args, va_list ap);

/* Vprec Function Instrumentation exit function */
void _vfi_exit_function(interflop_function_stack_t *stack, void *context,
                        int nb_args, va_list ap);

#endif /* __INTERFLOP_VPREC_FUNCTION_INSTRUMENTATION_H__ */