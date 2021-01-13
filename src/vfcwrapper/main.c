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

typedef struct interflop_backend_interface_t (*interflop_init_t)(
    int argc, char **argv, void **context);

#define MAX_BACKENDS 16
#define MAX_ARGS 256

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

static char *dd_exclude_path = NULL;
static char *dd_include_path = NULL;
static char *dd_generate_path = NULL;

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
  for (int i = 0; i < map->capacity; i++) {
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
    if (backends[i].interflop_exit_function)
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

/* Checks that a least one of the loaded backend implements the chosen
 * vector operation at a given precision */
#define check_backends_implements_vector(precision, operation)		\
  do {									\
    int res = 0;							\
    for (unsigned char i = 0; i < loaded_backends; i++) {		\
      if (backends[i].interflop_##operation##_##precision##_vector) {	\
        res = 1;							\
        break;								\
      } 								\
    }									\
    if (res == 0)							\
      logger_error("No backend instruments vector " #operation		\
		   " for " #precision					\
                   ".\n"						\
		   "Include one backend in VFC_BACKENDS "		\
		   "that provides it");					\
  } while (0)

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

  /* Parse VFC_BACKENDS */
  char *vfc_backends = getenv("VFC_BACKENDS");
  if (vfc_backends == NULL) {
    logger_error(
        "VFC_BACKENDS is empty, at least one backend should be provided");
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
        logger_error("VFC_BACKENDS syntax error: too many arguments");
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
    logger_error(
        "VFC_BACKENDS syntax error: at least one backend should be provided");
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

  /* Check that at least one backend implements each required vector 
   * operation */
  check_backends_implements_vector(float, add);
  check_backends_implements_vector(float, sub);
  check_backends_implements_vector(float, mul);
  check_backends_implements_vector(float, div);
  check_backends_implements_vector(double, add);
  check_backends_implements_vector(double, sub);
  check_backends_implements_vector(double, mul);
  check_backends_implements_vector(double, div);
#ifdef INST_FCMP
  check_backends_implements_vector(float, cmp);
  check_backends_implements_vector(double, cmp);
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

#define define_vector_arithmetic_wrapper(size, precision, operation,	\
					 operator)			\
  precision##size _##size##x##precision##operation(precision##size a,	\
						   precision##size b) {	\
    precision##size c = NAN;						\
    ddebug(operator);							\
    for (unsigned char i = 0; i < loaded_backends; i++) {		\
      if (backends[i].interflop_##operation##_##precision##_vector) {	\
        backends[i].interflop_##operation##_##precision##		\
	  _vector(size,							\
		  (precision *)(&a),					\
		  (precision *)(&b),					\
		  (precision *)(&c),					\
		  contexts[i]);						\
      } 								\
    }   								\
    return c;								\
  }

// Define wrapper for vector arithmetic operation for size 2
define_vector_arithmetic_wrapper(2, float, add, +);
define_vector_arithmetic_wrapper(2, float, sub, -);
define_vector_arithmetic_wrapper(2, float, mul, *);
define_vector_arithmetic_wrapper(2, float, div, /);
define_vector_arithmetic_wrapper(2, double, add, +);
define_vector_arithmetic_wrapper(2, double, sub, -);
define_vector_arithmetic_wrapper(2, double, mul, *);
define_vector_arithmetic_wrapper(2, double, div, /);

// Define wrapper for vector arithmetic operation for size 4
define_vector_arithmetic_wrapper(4, float, add, +);
define_vector_arithmetic_wrapper(4, float, sub, -);
define_vector_arithmetic_wrapper(4, float, mul, *);
define_vector_arithmetic_wrapper(4, float, div, /);
define_vector_arithmetic_wrapper(4, double, add, +);
define_vector_arithmetic_wrapper(4, double, sub, -);
define_vector_arithmetic_wrapper(4, double, mul, *);
define_vector_arithmetic_wrapper(4, double, div, /);

// Define wrapper for vector arithmetic operation for size 8
define_vector_arithmetic_wrapper(8, float, add, +);
define_vector_arithmetic_wrapper(8, float, sub, -);
define_vector_arithmetic_wrapper(8, float, mul, *);
define_vector_arithmetic_wrapper(8, float, div, /);
define_vector_arithmetic_wrapper(8, double, add, +);
define_vector_arithmetic_wrapper(8, double, sub, -);
define_vector_arithmetic_wrapper(8, double, mul, *);
define_vector_arithmetic_wrapper(8, double, div, /);

// Define wrapper for vector arithmetic operation for size 16
define_vector_arithmetic_wrapper(16, float, add, +);
define_vector_arithmetic_wrapper(16, float, sub, -);
define_vector_arithmetic_wrapper(16, float, mul, *);
define_vector_arithmetic_wrapper(16, float, div, /);
define_vector_arithmetic_wrapper(16, double, add, +);
define_vector_arithmetic_wrapper(16, double, sub, -);
define_vector_arithmetic_wrapper(16, double, mul, *);
define_vector_arithmetic_wrapper(16, double, div, /);

/* Comparison vector wrappers */

#define define_vector_cmp_wrapper(size, precision)			\
  int##size _##size##x##precision##cmp(enum FCMP_PREDICATE p,		\
				       precision##size a,		\
				       precision##size b) {		\
    int##size c;							\
    for (unsigned char i = 0; i < loaded_backends; i++) {		\
      if (backends[i].interflop_cmp_##precision##_vector) {		\
	backends[i].interflop_cmp_##precision##				\
	  _vector(p, size,						\
		  (precision *)(&a),					\
		  (precision *)(&b),					\
		  (int *)(&c),						\
		  contexts[i]);						\
      } 								\
    }   								\
    return c;								\
  }

define_vector_cmp_wrapper(2, float);
define_vector_cmp_wrapper(2, double);

define_vector_cmp_wrapper(4, float);
define_vector_cmp_wrapper(4, double);

define_vector_cmp_wrapper(8, float);
define_vector_cmp_wrapper(8, double);

define_vector_cmp_wrapper(16, float);
define_vector_cmp_wrapper(16, double);
