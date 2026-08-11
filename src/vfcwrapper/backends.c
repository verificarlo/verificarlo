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

#include <math.h>
#include <stdarg.h>

#include "interflop/hashmap/vfc_hashmap.h"
#include "interflop/interflop.h"

#include "backends.h"
#include "vector_ext.h"

#define DO_PRAGMA(x) _Pragma(#x)

#if defined(__clang__)
#define UNROLL(n) DO_PRAGMA(clang loop unroll_count(n))
#elif defined(__GNUC__)
#define UNROLL(n) DO_PRAGMA(GCC unroll n)
#else
#define UNROLL(n)
#endif

/* --- Global backend state ------------------------------------------------ */

struct interflop_backend_interface_t backends[MAX_BACKENDS];
void *contexts[MAX_BACKENDS] = {NULL};
unsigned char loaded_backends = 0;

/* --- Delta-debug state --------------------------------------------------- */

unsigned char ddebug_enabled = 0;
char *dd_exclude_path = NULL;
char *dd_include_path = NULL;
char *dd_generate_path = NULL;
vfc_hashmap_t dd_must_instrument;
vfc_hashmap_t dd_mustnot_instrument;

/* --- Delta-debug filter macro -------------------------------------------- */
/* When delta-debug run flags are passed, check filter rules.
 * Exclude rules are applied first and have priority. */
#define ddebug(operation)                                                      \
  if (ddebug_enabled) {                                                        \
    void *addr = __builtin_return_address(0);                                  \
    if (dd_exclude_path) {                                                     \
      /* Ignore addr in exclude file */                                        \
      if (vfc_hashmap_have(dd_mustnot_instrument, (size_t)addr)) {             \
        return operation;                                                      \
      }                                                                        \
    }                                                                          \
    if (dd_include_path) {                                                     \
      /* Ignore addr not in include file */                                    \
      if (!vfc_hashmap_have(dd_must_instrument, (size_t)addr)) {               \
        return operation;                                                      \
      }                                                                        \
    } else if (dd_generate_path) {                                             \
      vfc_hashmap_insert(dd_must_instrument, (size_t)addr, addr);              \
    }                                                                          \
  }

/* --- User-call dispatch -------------------------------------------------- */

void interflop_call(interflop_call_id id, ...) {
  va_list ap;
  for (unsigned char i = 0; i < loaded_backends; i++) {
    if (backends[i].interflop_user_call) {
      va_start(ap, id);
      backends[i].interflop_user_call(contexts[i], id, ap);
      va_end(ap);
    }
  }
}

/* --- Scalar arithmetic dispatch ------------------------------------------ */

#define define_arithmetic_wrapper(precision, operation, operator)              \
  precision _##precision##operation(precision a, precision b) {                \
    precision c = NAN;                                                         \
    ddebug(operator);                                                          \
    for (unsigned char i = 0; i < loaded_backends; i++) {                      \
      if (backends[i].interflop_##operation##_##precision) {                   \
        backends[i].interflop_##operation##_##precision(a, b, &c,              \
                                                        contexts[i]);          \
      }                                                                        \
    }                                                                          \
    return c;                                                                  \
  }

define_arithmetic_wrapper(float, add, (a + b));
define_arithmetic_wrapper(float, sub, (a - b));
define_arithmetic_wrapper(float, mul, (a * b));
define_arithmetic_wrapper(float, div, (a / b));
define_arithmetic_wrapper(double, add, (a + b));
define_arithmetic_wrapper(double, sub, (a - b));
define_arithmetic_wrapper(double, mul, (a * b));
define_arithmetic_wrapper(double, div, (a / b));

/* --- Scalar comparison dispatch ------------------------------------------ */

int _floatcmp(enum FCMP_PREDICATE p, float a, float b) {
  int c = 0;
  for (unsigned int i = 0; i < loaded_backends; i++) {
    if (backends[i].interflop_cmp_float) {
      backends[i].interflop_cmp_float(p, a, b, &c, contexts[i]);
    }
  }
  return c;
}

int _doublecmp(enum FCMP_PREDICATE p, double a, double b) {
  int c = 0;
  for (unsigned int i = 0; i < loaded_backends; i++) {
    if (backends[i].interflop_cmp_double) {
      backends[i].interflop_cmp_double(p, a, b, &c, contexts[i]);
    }
  }
  return c;
}

/* --- Scalar FMA dispatch ------------------------------------------------- */

#define define_arithmetic_fma_wrapper(precision)                               \
  precision _##precision##fma(precision a, precision b, precision c) {         \
    precision d = NAN;                                                         \
    ddebug(((a * b) + c));                                                     \
    for (unsigned char i = 0; i < loaded_backends; i++) {                      \
      if (backends[i].interflop_fma_##precision) {                             \
        backends[i].interflop_fma_##precision(a, b, c, &d, contexts[i]);       \
      }                                                                        \
    }                                                                          \
    return d;                                                                  \
  }

define_arithmetic_fma_wrapper(float);
define_arithmetic_fma_wrapper(double);

/* --- Cast dispatch ------------------------------------------------------- */

float _doubletofloatcast(double a) {
  float b = NAN;
  for (unsigned int i = 0; i < loaded_backends; i++) {
    if (backends[i].interflop_cast_double_to_float) {
      backends[i].interflop_cast_double_to_float(a, &b, contexts[i]);
    }
  }
  return b;
}

/* --- Generic (non-ISA-specific) vector arithmetic dispatch --------------- */

#define define_vectorized_arithmetic_wrapper(precision, operation, size)       \
  precision##size _##size##x##precision##operation(const precision##size a,    \
                                                   const precision##size b) {  \
    typedef union {                                                            \
      precision##size v;                                                       \
      precision a[size];                                                       \
    } fvec;                                                                    \
    fvec au = {.v = a}, bu = {.v = b}, cu;                                     \
    for (int i = 0; i < loaded_backends; i++) {                                \
      if (backends[i].interflop_##operation##_##precision) {                   \
        UNROLL(size)                                                           \
        for (int j = 0; j < (size); j++) {                                     \
          backends[i].interflop_##operation##_##precision(                     \
              au.a[j], bu.a[j], &(cu.a[j]), contexts[i]);                      \
        }                                                                      \
      }                                                                        \
    }                                                                          \
    return cu.v;                                                               \
  }

/* Define vector of size 2 */
define_vectorized_arithmetic_wrapper(float, add, 2);
define_vectorized_arithmetic_wrapper(float, sub, 2);
define_vectorized_arithmetic_wrapper(float, mul, 2);
define_vectorized_arithmetic_wrapper(float, div, 2);
define_vectorized_arithmetic_wrapper(double, add, 2);
define_vectorized_arithmetic_wrapper(double, sub, 2);
define_vectorized_arithmetic_wrapper(double, mul, 2);
define_vectorized_arithmetic_wrapper(double, div, 2);

/* Define vector of size 4 */
define_vectorized_arithmetic_wrapper(float, add, 4);
define_vectorized_arithmetic_wrapper(float, sub, 4);
define_vectorized_arithmetic_wrapper(float, mul, 4);
define_vectorized_arithmetic_wrapper(float, div, 4);
define_vectorized_arithmetic_wrapper(double, add, 4);
define_vectorized_arithmetic_wrapper(double, sub, 4);
define_vectorized_arithmetic_wrapper(double, mul, 4);
define_vectorized_arithmetic_wrapper(double, div, 4);

/* Define vector of size 8 */
define_vectorized_arithmetic_wrapper(float, add, 8);
define_vectorized_arithmetic_wrapper(float, sub, 8);
define_vectorized_arithmetic_wrapper(float, mul, 8);
define_vectorized_arithmetic_wrapper(float, div, 8);
define_vectorized_arithmetic_wrapper(double, add, 8);
define_vectorized_arithmetic_wrapper(double, sub, 8);
define_vectorized_arithmetic_wrapper(double, mul, 8);
define_vectorized_arithmetic_wrapper(double, div, 8);

/* Define vector of size 16 */
define_vectorized_arithmetic_wrapper(float, add, 16);
define_vectorized_arithmetic_wrapper(float, sub, 16);
define_vectorized_arithmetic_wrapper(float, mul, 16);
define_vectorized_arithmetic_wrapper(float, div, 16);
define_vectorized_arithmetic_wrapper(double, add, 16);
define_vectorized_arithmetic_wrapper(double, sub, 16);
define_vectorized_arithmetic_wrapper(double, mul, 16);
define_vectorized_arithmetic_wrapper(double, div, 16);

/* --- Generic vector comparison dispatch ---------------------------------- */

#define define_vectorized_comparison_wrapper(precision, size)                  \
  int##size _##size##x##precision##cmp(enum FCMP_PREDICATE p,                  \
                                       precision##size a, precision##size b) { \
    typedef union {                                                            \
      int##size v;                                                             \
      int a[size];                                                             \
    } ivec;                                                                    \
    typedef union {                                                            \
      precision##size v;                                                       \
      precision a[size];                                                       \
    } fvec;                                                                    \
    fvec au = {.v = a}, bu = {.v = b};                                         \
    ivec cu;                                                                   \
    for (int i = 0; i < loaded_backends; i++) {                                \
      if (backends[i].interflop_cmp_##precision) {                             \
        UNROLL(size) for (int j = 0; j < (size); j++) {                        \
          backends[i].interflop_cmp_##precision(p, au.a[j], bu.a[j],           \
                                                &(cu.a[j]), contexts[i]);      \
        }                                                                      \
      }                                                                        \
    }                                                                          \
    return cu.v;                                                               \
  }

define_vectorized_comparison_wrapper(float, 2);
define_vectorized_comparison_wrapper(double, 2);

define_vectorized_comparison_wrapper(float, 4);
define_vectorized_comparison_wrapper(double, 4);

define_vectorized_comparison_wrapper(float, 8);
define_vectorized_comparison_wrapper(double, 8);

define_vectorized_comparison_wrapper(float, 16);
define_vectorized_comparison_wrapper(double, 16);

/* --- Generic vector FMA dispatch ----------------------------------------- */

#define define_vectorized_fma_wrapper(precision, size)                         \
  precision##size _##size##x##precision##fma(const precision##size a,          \
                                             const precision##size b,          \
                                             const precision##size c) {        \
    typedef union {                                                            \
      precision##size v;                                                       \
      precision a[size];                                                       \
    } fvec;                                                                    \
    fvec au = {.v = a}, bu = {.v = b}, cu = {.v = c}, du;                      \
    for (int i = 0; i < loaded_backends; i++) {                                \
      if (backends[i].interflop_fma_##precision) {                             \
        UNROLL(size)                                                           \
        for (int j = 0; j < (size); j++) {                                     \
          backends[i].interflop_fma_##precision(au.a[j], bu.a[j], cu.a[j],     \
                                                &(du.a[j]), contexts[i]);      \
        }                                                                      \
      }                                                                        \
    }                                                                          \
    return du.v;                                                               \
  }

define_vectorized_fma_wrapper(float, 2);
define_vectorized_fma_wrapper(float, 4);
define_vectorized_fma_wrapper(float, 8);
define_vectorized_fma_wrapper(float, 16);

define_vectorized_fma_wrapper(double, 2);
define_vectorized_fma_wrapper(double, 4);
define_vectorized_fma_wrapper(double, 8);
define_vectorized_fma_wrapper(double, 16);

/* --- ISA-specific vector wrappers --------------------------------------- */
/* Included after the scalar dispatch functions they depend on.            */

#if defined(__x86_64__)
#include "operation_x86.h"
#elif defined(__aarch64__)
#include "operation_arm.h"
#elif defined(__amd64__)
#include "operation_amd.h"
#endif