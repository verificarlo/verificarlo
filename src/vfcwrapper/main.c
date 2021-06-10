/*****************************************************************************
 *                                                                           *
 *  This file is part of Verificarlo.                                        *
 *                                                                           *
 *  Copyright (c) 2015-2020                                                  *
 *     Verificarlo contributors                                              *
 *     Universite de Versailles St-Quentin-en-Yvelines                       *
 *     CMLA, Ecole Normale Superieure de Cachan                              *
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

#include <assert.h>
#include <dlfcn.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "interflop.h"

/* In delta-debug we retrieve the return address of
 * instrumented operations. Call op size allows us
 * to compute the previous instruction so that the
 * user sees the address of the actual operation */
#ifdef __x86_64__
#define CALL_OP_SIZE 5
#else
/* On other architectures we assume an instruction is
 * 4 bytes */
#define CALL_OP_SIZE 4
#endif

typedef int int2 __attribute__((ext_vector_type(2)));
typedef int int4 __attribute__((ext_vector_type(4)));
typedef int int8 __attribute__((ext_vector_type(8)));
typedef int int16 __attribute__((ext_vector_type(16)));

typedef float float2 __attribute__((ext_vector_type(2)));
typedef float float4 __attribute__((ext_vector_type(4)));
typedef float float8 __attribute__((ext_vector_type(8)));
typedef float float16 __attribute__((ext_vector_type(16)));

typedef double double2 __attribute__((ext_vector_type(2)));
typedef double double4 __attribute__((ext_vector_type(4)));
typedef double double8 __attribute__((ext_vector_type(8)));
typedef double double16 __attribute__((ext_vector_type(16)));

typedef struct interflop_backend_interface_t (*interflop_init_t)(
    int argc, char **argv, void **context);

#define MAX_BACKENDS 16
#define MAX_ARGS 256

#define XSTR(X) STR(X)
#define STR(X) #X

struct interflop_backend_interface_t backends[MAX_BACKENDS];
void *contexts[MAX_BACKENDS];
unsigned char loaded_backends = 0;
unsigned char already_initialized = 0;

/* Logger functions */
#undef BACKEND_HEADER
#define BACKEND_HEADER verificarlo
void logger_init(void);
void logger_info(const char *fmt, ...);
void logger_warning(const char *fmt, ...);
void logger_error(const char *fmt, ...);

__attribute__((unused)) static char *dd_exclude_path = NULL;
__attribute__((unused)) static char *dd_include_path = NULL;
__attribute__((unused)) static char *dd_generate_path = NULL;

/* Function instrumentation prototypes */

void vfc_init_func_inst();

void vfc_quit_func_inst();

/* Hashmap header */

#define __VFC_HASHMAP_HEADER__

struct vfc_hashmap_st {
  size_t nbits;
  size_t mask;

  size_t capacity;
  size_t *items;
  size_t nitems;
  size_t n_deleted_items;
};

typedef struct vfc_hashmap_st *vfc_hashmap_t;
// allocate and initialize the map
vfc_hashmap_t vfc_hashmap_create();

// free the map
void vfc_hashmap_destroy(vfc_hashmap_t map);

// get the value at an index of a map
size_t get_value_at(size_t *items, size_t i);

// get the key at an index of a map
size_t get_key_at(size_t *items, size_t i);

// set the value at an index of a map
void set_value_at(size_t *items, size_t value, size_t i);

// set the key at an index of a map
void set_key_at(size_t *items, size_t key, size_t i);

// insert an element in the map
void vfc_hashmap_insert(vfc_hashmap_t map, size_t key, void *item);

// remove an element of the map
void vfc_hashmap_remove(vfc_hashmap_t map, size_t key);

// test if an element is in the map
char vfc_hashmap_have(vfc_hashmap_t map, size_t key);

// get an element of the map
void *vfc_hashmap_get(vfc_hashmap_t map, size_t key);

// get the number of elements in the map
size_t vfc_hashmap_num_items(vfc_hashmap_t map);

// Hash function for strings
size_t vfc_hashmap_str_function(const char *id);

// Free the hashmap
void vfc_hashmap_free(vfc_hashmap_t map);

/* dd_must_instrument is used to apply and generate include DD filters */
/* dd_mustnot_instrument is used to apply exclude DD filters */
vfc_hashmap_t dd_must_instrument;
vfc_hashmap_t dd_mustnot_instrument;

void ddebug_generate_inclusion(char *dd_generate_path, vfc_hashmap_t map) {
  int output = open(dd_generate_path, O_WRONLY | O_CREAT, S_IWUSR | S_IRUSR);
  if (output == -1) {
    logger_error("cannot open DDEBUG_GEN file %s", dd_generate_path);
  }
  for (size_t i = 0; i < map->capacity; i++) {
    if (get_value_at(map->items, i) != 0 && get_value_at(map->items, i) != 1) {
      pid_t pid = fork();
      if (pid == 0) {
        char addr[19];
        char executable[64];
        snprintf(addr, 19, "%p",
                 (void *)(get_value_at(map->items, i) - CALL_OP_SIZE));
        snprintf(executable, 64, "/proc/%d/exe", getppid());
        dup2(output, 1);
        execlp("addr2line", "/usr/bin/addr2line", "-fpaCs", "-e", executable,
               addr, NULL);
        logger_error("error running addr2line");
      } else {
        int status;
        wait(&status);
        assert(status == 0);
      }
    }
  }
  close(output);
}

__attribute__((destructor(0))) static void vfc_atexit(void) {

  /* Send finalize message to backends */
  for (int i = 0; i < loaded_backends; i++)
    if (backends[i].interflop_finalize)
      backends[i].interflop_finalize(contexts[i]);

#ifdef DDEBUG
  if (dd_generate_path) {
    ddebug_generate_inclusion(dd_generate_path, dd_must_instrument);
    logger_info("ddebug: generated complete inclusion file at %s\n",
                dd_generate_path);
  }
  vfc_hashmap_destroy(dd_must_instrument);
  vfc_hashmap_destroy(dd_mustnot_instrument);
#endif

#ifdef INST_FUNC
  vfc_quit_func_inst();
#endif
}

/* Checks that a least one of the loaded backend implements the chosen
 * operation at a given precision */
#define check_backends_implements(precision, operation)                        \
  do {                                                                         \
    int res = 0;                                                               \
    for (unsigned char i = 0; i < loaded_backends; i++) {                      \
      if (backends[i].interflop_##operation##_##precision) {                   \
        res = 1;                                                               \
        break;                                                                 \
      }                                                                        \
    }                                                                          \
    if (res == 0)                                                              \
      logger_error("No backend instruments " #operation " for " #precision     \
                   ".\n"                                                       \
                   "Include one backend in VFC_BACKENDS that provides it");    \
  } while (0)

#if DDEBUG
/* vfc_read_filter_file reads an inclusion/exclusion ddebug file and returns
 * an address map */
static void vfc_read_filter_file(const char *dd_filter_path,
                                 vfc_hashmap_t map) {
  FILE *input = fopen(dd_filter_path, "r");
  if (input) {
    void *addr;
    char line[2048];
    int lineno = 0;
    while (fgets(line, sizeof line, input)) {
      lineno++;
      if (sscanf(line, "%p", &addr) == 1) {
        vfc_hashmap_insert(map, (size_t)addr + CALL_OP_SIZE,
                           addr + CALL_OP_SIZE);
      } else {
        logger_error(
            "ddebug: error parsing VFC_DDEBUG_[INCLUDE/EXCLUDE] %s at line %d",
            dd_filter_path, lineno);
      }
    }
  }
}
#endif

/* Parse the different VFC_BACKENDS variables per priorty order */
/* 1- VFC_BACKENDS */
/* 2- VFC_BACKENDS_FROM_FILE */
/* Set the backends read in vfc_backends */
/* Set the name of the environment variable read in vfc_backends_env */
void parse_vfc_backends_env(char **vfc_backends, char **vfc_backends_env) {

  /* Parse VFC_BACKENDS */
  *vfc_backends_env = (char *)malloc(sizeof(char) * 256);
  *vfc_backends = (char *)malloc(sizeof(char) * 256);

  sprintf(*vfc_backends_env, "VFC_BACKENDS");
  *vfc_backends = getenv(*vfc_backends_env);

  /* Parse VFC_BACKENDS_FROM_FILE if VFC_BACKENDS is empty*/
  if (*vfc_backends == NULL) {
    sprintf(*vfc_backends_env, "VFC_BACKENDS_FROM_FILE");
    char *vfc_backends_fromfile_file = getenv(*vfc_backends_env);
    if (vfc_backends_fromfile_file != NULL) {
      FILE *fi = fopen(vfc_backends_fromfile_file, "r");
      if (fi == NULL) {
        logger_error("Error while opening file pointed by %s: %s",
                     *vfc_backends_env, strerror(errno));
      } else {
        size_t len = 0;
        ssize_t nread;
        nread = getline(vfc_backends, &len, fi);
        if (nread == -1) {
          logger_error("Error while reading file pointed by %s: %s",
                       *vfc_backends_env, strerror(errno));
        } else {
          if ((*vfc_backends)[nread - 1] == '\n') {
            (*vfc_backends)[nread - 1] = '\0';
          }
        }
      }
    }
  }
}

/* vfc_init is run when loading vfcwrapper and initializes vfc backends */
__attribute__((constructor(0))) static void vfc_init(void) {

  /* The vfcwrapper library constructor may be loaded multiple times.  This
   * happends for example, when a .so compiled with Verificarlo is loaded with
   * dlopen into another program also compiled with Verificarlo.
   *
   * The following hook should ensure that vfc_init is loaded only once.
   * Is this code robust? dlopen is thread safe, so this should work.
   *
   */
  if (already_initialized == 0) {
    already_initialized = 1;
  } else {
    return;
  }

  /* Initialize instumentation */
#ifdef INST_FUNC
  vfc_init_func_inst();
#endif

  /* Initialize the logger */
  logger_init();

  char *vfc_backends = NULL, *vfc_backends_env = NULL;
  parse_vfc_backends_env(&vfc_backends, &vfc_backends_env);

  if (vfc_backends == NULL) {
    logger_error("%s is empty, at least one backend should be provided",
                 vfc_backends_env);
  }

  /* Environnement variable to disable loading message */
  char *silent_load_env = getenv("VFC_BACKENDS_SILENT_LOAD");
  bool silent_load =
      ((silent_load_env == NULL) || (strcasecmp(silent_load_env, "True") != 0))
          ? false
          : true;

  /* For each backend, load and register the backend vtable interface
     Backends .so are separated by semi-colons in the VFC_BACKENDS
     env variable */
  char *semicolonptr;
  char *token = strtok_r(vfc_backends, ";", &semicolonptr);
  while (token) {
    /* Parse each backend arguments, argv[0] is the backend name */
    int backend_argc = 0;
    char *backend_argv[MAX_ARGS];
    char *spaceptr;
    char *arg = strtok_r(token, " ", &spaceptr);
    while (arg) {
      if (backend_argc >= MAX_ARGS) {
        logger_error("%s syntax error: too many arguments", vfc_backends_env);
      }
      backend_argv[backend_argc++] = arg;
      arg = strtok_r(NULL, " ", &spaceptr);
    }
    backend_argv[backend_argc] = NULL;

    /* load the backend .so */
    void *handle = dlopen(backend_argv[0], RTLD_NOW);
    if (handle == NULL) {
      logger_error("Cannot load backend %s: dlopen error\n%s", token,
                   dlerror());
    }

    if (!silent_load)
      logger_info("loaded backend %s\n", token);

    /* reset dl errors */
    dlerror();

    /* get the address of the interflop_init function */
    interflop_init_t handle_init =
        (interflop_init_t)dlsym(handle, "interflop_init");
    const char *dlsym_error = dlerror();
    if (dlsym_error) {
      logger_error("No interflop_init function in backend %s: %s", token,
                   strerror(errno));
    }

    /* Register backend */
    if (loaded_backends == MAX_BACKENDS) {
      logger_error("No more than %d backends can be used simultaneously",
                   MAX_BACKENDS);
    }
    backends[loaded_backends] =
        handle_init(backend_argc, backend_argv, &contexts[loaded_backends]);
    loaded_backends++;

    /* parse next backend token */
    token = strtok_r(NULL, ";", &semicolonptr);
  }

  if (loaded_backends == 0) {
    logger_error("%s syntax error: at least one backend should be provided",
                 vfc_backends_env);
  }

  /* Check that at least one backend implements each required operation */
  check_backends_implements(float, add);
  check_backends_implements(float, sub);
  check_backends_implements(float, mul);
  check_backends_implements(float, div);
  check_backends_implements(double, add);
  check_backends_implements(double, sub);
  check_backends_implements(double, mul);
  check_backends_implements(double, div);
#ifdef INST_FCMP
  check_backends_implements(float, cmp);
  check_backends_implements(double, cmp);
#endif

#ifdef DDEBUG
  /* Initialize ddebug */
  dd_must_instrument = vfc_hashmap_create();
  dd_mustnot_instrument = vfc_hashmap_create();
  dd_exclude_path = getenv("VFC_DDEBUG_EXCLUDE");
  dd_include_path = getenv("VFC_DDEBUG_INCLUDE");
  dd_generate_path = getenv("VFC_DDEBUG_GEN");
  if (dd_include_path && dd_generate_path) {
    logger_error(
        "VFC_DDEBUG_INCLUDE and VFC_DDEBUG_GEN should not be both defined "
        "at the same time");
  }
  if (dd_include_path) {
    vfc_read_filter_file(dd_include_path, dd_must_instrument);
    logger_info("ddebug: only %zu addresses will be instrumented\n",
                vfc_hashmap_num_items(dd_must_instrument));
  }
  if (dd_exclude_path) {
    vfc_read_filter_file(dd_exclude_path, dd_mustnot_instrument);
    logger_info("ddebug: %zu addresses will not be instrumented\n",
                vfc_hashmap_num_items(dd_mustnot_instrument));
  }
#endif
}

/* Arithmetic wrappers */
#ifdef DDEBUG
/* When delta-debug run flags are passed, check filter rules,
 *  - exclude rules are applied first and have priority
 * */
#define ddebug(operator)                                                       \
  void *addr = __builtin_return_address(0);                                    \
  if (dd_exclude_path) {                                                       \
    /* Ignore addr in exclude file */                                          \
    if (vfc_hashmap_have(dd_mustnot_instrument, (size_t)addr)) {               \
      return a operator b;                                                     \
    }                                                                          \
  }                                                                            \
  if (dd_include_path) {                                                       \
    /* Ignore addr not in include file */                                      \
    if (!vfc_hashmap_have(dd_must_instrument, (size_t)addr)) {                 \
      return a operator b;                                                     \
    }                                                                          \
  } else if (dd_generate_path) {                                               \
    vfc_hashmap_insert(dd_must_instrument, (size_t)addr, addr);                \
  }

#else
/* When delta-debug flags are not passed do nothing */
#define ddebug(operator)                                                       \
  do {                                                                         \
  } while (0)
#endif

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

define_arithmetic_wrapper(float, add, +);
define_arithmetic_wrapper(float, sub, -);
define_arithmetic_wrapper(float, mul, *);
define_arithmetic_wrapper(float, div, /);
define_arithmetic_wrapper(double, add, +);
define_arithmetic_wrapper(double, sub, -);
define_arithmetic_wrapper(double, mul, *);
define_arithmetic_wrapper(double, div, /);

int _floatcmp(enum FCMP_PREDICATE p, float a, float b) {
  int c;
  for (unsigned int i = 0; i < loaded_backends; i++) {
    if (backends[i].interflop_cmp_float) {
      backends[i].interflop_cmp_float(p, a, b, &c, contexts[i]);
    }
  }
  return c;
}

int _doublecmp(enum FCMP_PREDICATE p, double a, double b) {
  int c;
  for (unsigned int i = 0; i < loaded_backends; i++) {
    if (backends[i].interflop_cmp_double) {
      backends[i].interflop_cmp_double(p, a, b, &c, contexts[i]);
    }
  }
  return c;
}

/* Arithmetic vector wrappers */
#define define_vectorized_arithmetic_wrapper(precision, operation, size)       \
  precision##size _##size##x##precision##operation(const precision##size a,    \
                                                   const precision##size b) {  \
    precision##size c;                                                         \
                                                                               \
    _Pragma("unroll") for (int i = 0; i < size; i++) {                         \
      c[i] = _##precision##operation(a[i], b[i]);                              \
    }                                                                          \
    return c;                                                                  \
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

/* Comparison vector wrappers */
#define define_vectorized_comparison_wrapper(precision, size)                  \
  int##size _##size##x##precision##cmp(enum FCMP_PREDICATE p,                  \
                                       precision##size a, precision##size b) { \
    int##size c;                                                               \
    _Pragma("unroll") for (int i = 0; i < size; i++) {                         \
      c[i] = _##precision##cmp(p, a[i], b[i]);                                 \
    }                                                                          \
    return c;                                                                  \
  }

define_vectorized_comparison_wrapper(float, 2);
define_vectorized_comparison_wrapper(double, 2);

define_vectorized_comparison_wrapper(float, 4);
define_vectorized_comparison_wrapper(double, 4);

define_vectorized_comparison_wrapper(float, 8);
define_vectorized_comparison_wrapper(double, 8);

define_vectorized_comparison_wrapper(float, 16);
define_vectorized_comparison_wrapper(double, 16);
