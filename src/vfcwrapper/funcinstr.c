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
 *  Copyright (c) 2019-2024                                                  *\
 *     Verificarlo Contributors                                              *\
 *                                                                           *\
 ****************************************************************************/

#include <stdio.h>

#include "interflop/hashmap/vfc_hashmap.h"
#include "interflop/interflop_stdlib.h"
#include "interflop/iostream/logger.h"

#include "backends.h"
#include "funcinstr.h"

#define _VFC_CALL_STACK_MAXSIZE 4096

/************************************************************
 *                       Hash Functions                     *
 ************************************************************/
vfc_hashmap_t _vfc_func_map;

// Add a function in the hash table
interflop_function_info_t *
vfc_func_table_add(interflop_function_info_t function) {
  size_t key = vfc_hashmap_str_function(function.id);

  interflop_function_info_t *ptr =
      (interflop_function_info_t *)malloc(sizeof(interflop_function_info_t));

  (*ptr) = function;

  vfc_hashmap_insert(_vfc_func_map, key, (void *)ptr);

  return ptr;
}

// Search a function in the hash table
interflop_function_info_t *vfc_func_table_get(const char *id) {
  size_t key = vfc_hashmap_str_function(id);

  return vfc_hashmap_get(_vfc_func_map, key);
}

// Print the table
void _vfc_func_table_print(FILE *f) {
  for (size_t ii = 0; ii < _vfc_func_map->capacity; ii++) {
    if (get_value_at(_vfc_func_map->items, ii) != 0 &&
        get_value_at(_vfc_func_map->items, ii) != 0) {
      interflop_function_info_t *function =
          (interflop_function_info_t *)get_value_at(_vfc_func_map->items, ii);
      interflop_fprintf(f, "%s\t%hd\t%hd\t%hu\t%hu\n", function->id,
                        function->isLibraryFunction,
                        function->isIntrinsicFunction, function->useFloat,
                        function->useDouble);
    }
  }
}

void vfc_func_table_init() { _vfc_func_map = vfc_hashmap_create(); }

void vfc_func_table_quit() {
  vfc_hashmap_free(_vfc_func_map);

  vfc_hashmap_destroy(_vfc_func_map);
}

/************************************************************
 *                       Call Stack                         *
 ************************************************************/
static interflop_function_stack_t _vfc_call_stack = {NULL,
                                                     _VFC_CALL_STACK_MAXSIZE};

// Initialize the call stack
void vfc_call_stack_init() {
  _vfc_call_stack.array = interflop_malloc(_VFC_CALL_STACK_MAXSIZE *
                                           sizeof(interflop_function_info_t *));
  _vfc_call_stack.array[--_vfc_call_stack.top] = NULL;
}

// Push a function in the call stack
void vfc_call_stack_push(interflop_function_info_t *function) {
  if (_vfc_call_stack.top == 0) {
    logger_error("Call stack is full, it max size is %zu\n",
                 _VFC_CALL_STACK_MAXSIZE);
    return;
  }

  _vfc_call_stack.array[--_vfc_call_stack.top] = function;
}

// Remove a function in the call stack
interflop_function_info_t *vfc_call_stack_pop() {
  if (_vfc_call_stack.top < _VFC_CALL_STACK_MAXSIZE)
    return _vfc_call_stack.array[_vfc_call_stack.top++];

  return NULL;
}

// Print the call stack
void vfc_call_stack_print(FILE *f) {
  for (int i = _VFC_CALL_STACK_MAXSIZE - 2; i >= _vfc_call_stack.top; i--)
    interflop_fprintf(f, "%s/", _vfc_call_stack.array[i]->id);
  interflop_fprintf(f, "\n");
}

// Free the call stack
void vfc_call_stack_free() {
  if (_vfc_call_stack.array) {
    interflop_free(_vfc_call_stack.array);
  }
}

/************************************************************
 *                  Enter and Exit functions                *
 ************************************************************/

// Function called before each function's call of the code
void vfc_enter_function(char *func_name, char isLibraryFunction,
                        char isIntrinsicFunction, char useFloat, char useDouble,
                        int n, ...) {
  // Get a pointer to the function if she is in the table
  interflop_function_info_t *function = vfc_func_table_get(func_name);

  if (function == NULL) {
    interflop_function_info_t f = {.id = func_name,
                                   .isLibraryFunction = isLibraryFunction,
                                   .isIntrinsicFunction = isIntrinsicFunction,
                                   .useFloat = useFloat,
                                   .useDouble = useDouble};
    function = vfc_func_table_add(f);
  }

  vfc_call_stack_push(function);

  if ((useFloat != 0) || (useDouble != 0)) {
    va_list ap;
    // n is the number of arguments intercepted, each argument
    // is represented by a type ID and a pointer
    va_start(ap, n);

    for (int i = 0; i < loaded_backends; i++)
      if (backends[i].interflop_enter_function)
        backends[i].interflop_enter_function(&_vfc_call_stack, contexts[i], n,
                                             ap);

    va_end(ap);
  }
}

// Function called after each function's call of the code
void vfc_exit_function(__attribute__((unused)) char *func_name,
                       __attribute__((unused)) char isLibraryFunction,
                       __attribute__((unused)) char isIntrinsicFunction,
                       char useFloat, char useDouble, int n, ...) {

  if ((useFloat != 0) || (useDouble != 0)) {
    va_list ap;
    // n is the number of arguments intercepted, each argument
    // is represented by a type ID and a pointer
    va_start(ap, n);

    for (int i = 0; i < loaded_backends; i++)
      if (backends[i].interflop_exit_function)
        backends[i].interflop_exit_function(&_vfc_call_stack, contexts[i], n,
                                            ap);

    va_end(ap);
  }

  vfc_call_stack_pop();
}

/************************************************************
 *                   Init and Quit functions                *
 ************************************************************/

void vfc_init_func_inst() {
  // Initialize the call stack
  vfc_call_stack_init();

  // Initialize the hashmap
  vfc_func_table_init();
}

void vfc_quit_func_inst() {

  // Free the call stack
  vfc_call_stack_free();

  // Free the hashmap
  vfc_func_table_quit();
}
