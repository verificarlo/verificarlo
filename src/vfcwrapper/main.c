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

typedef double double2 __attribute__((ext_vector_type(2)));
typedef double double4 __attribute__((ext_vector_type(4)));
typedef double double8 __attribute__((ext_vector_type(8)));
typedef double double16 __attribute__((ext_vector_type(16)));
typedef float float2 __attribute__((ext_vector_type(2)));
typedef float float4 __attribute__((ext_vector_type(4)));
typedef float float8 __attribute__((ext_vector_type(8)));
typedef float float16 __attribute__((ext_vector_type(16)));
typedef int int2 __attribute__((ext_vector_type(2)));
typedef int int4 __attribute__((ext_vector_type(4)));
typedef int int8 __attribute__((ext_vector_type(8)));
typedef int int16 __attribute__((ext_vector_type(16)));

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

  /* Parse VFC_BACKENDS_FROM_FILE */
  char *vfc_backends_fromfile = NULL;
  char *vfc_backends_fromfile_file = getenv("VFC_BACKENDS_FROM_FILE");
  if (vfc_backends_fromfile_file != NULL) {
    FILE *fi = fopen(vfc_backends_fromfile_file, "r");
    if (fi == NULL) {
      logger_error(
          "Error while opening file pointed by VFC_BACKENDS_FROM_FILE: %s",
          strerror(errno));
    } else {
      size_t len = 0;
      ssize_t nread;
      nread = getline(&vfc_backends_fromfile, &len, fi);
      if (nread == -1) {
        logger_error(
            "Error while reading file pointed by VFC_BACKENDS_FROM_FILE: %s",
            strerror(errno));
      } else {
        if (vfc_backends_fromfile[nread - 1] == '\n') {
          vfc_backends_fromfile[nread - 1] = '\0';
        }
      }
    }
  }

  /* Parse VFC_BACKENDS */
  char *vfc_backends = getenv("VFC_BACKENDS");
  if (vfc_backends == NULL) {
    if (vfc_backends_fromfile == NULL) {
      logger_error(
          "VFC_BACKENDS is empty, at least one backend should be provided");
    } else {
      vfc_backends = vfc_backends_fromfile;
    }
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

#define define_2x_wrapper(precision, operation)                                \
  precision##2 _2x##precision##operation(precision##2 a, precision##2 b) {     \
    precision##2 c;                                                            \
    c[0] = _##precision##operation(a[0], b[0]);                                \
    c[1] = _##precision##operation(a[1], b[1]);                                \
    return c;                                                                  \
  }

#define define_4x_wrapper(precision, operation)                                \
  precision##4 _4x##precision##operation(precision##4 a, precision##4 b) {     \
    precision##4 c;                                                            \
    c[0] = _##precision##operation(a[0], b[0]);                                \
    c[1] = _##precision##operation(a[1], b[1]);                                \
    c[2] = _##precision##operation(a[2], b[2]);                                \
    c[3] = _##precision##operation(a[3], b[3]);                                \
    return c;                                                                  \
  }

#define define_8x_wrapper(precision, operation)                                \
  precision##8 _8x##precision##operation(precision##8 a, precision##8 b) {     \
    precision##8 c;                                                            \
    c[0] = _##precision##operation(a[0], b[0]);                                \
    c[1] = _##precision##operation(a[1], b[1]);                                \
    c[2] = _##precision##operation(a[2], b[2]);                                \
    c[3] = _##precision##operation(a[3], b[3]);                                \
    c[4] = _##precision##operation(a[4], b[4]);                                \
    c[5] = _##precision##operation(a[5], b[5]);                                \
    c[6] = _##precision##operation(a[6], b[6]);                                \
    c[7] = _##precision##operation(a[7], b[7]);                                \
    return c;                                                                  \
  }

#define define_16x_wrapper(precision, operation)                               \
  precision##16 _16x##precision##operation(precision##16 a, precision##16 b) { \
    precision##16 c;                                                           \
    c[0] = _##precision##operation(a[0], b[0]);                                \
    c[1] = _##precision##operation(a[1], b[1]);                                \
    c[2] = _##precision##operation(a[2], b[2]);                                \
    c[3] = _##precision##operation(a[3], b[3]);                                \
    c[4] = _##precision##operation(a[4], b[4]);                                \
    c[5] = _##precision##operation(a[5], b[5]);                                \
    c[6] = _##precision##operation(a[6], b[6]);                                \
    c[7] = _##precision##operation(a[7], b[7]);                                \
    c[8] = _##precision##operation(a[8], b[8]);                                \
    c[9] = _##precision##operation(a[9], b[9]);                                \
    c[10] = _##precision##operation(a[10], b[10]);                             \
    c[11] = _##precision##operation(a[11], b[11]);                             \
    c[12] = _##precision##operation(a[12], b[12]);                             \
    c[13] = _##precision##operation(a[13], b[13]);                             \
    c[14] = _##precision##operation(a[14], b[14]);                             \
    c[15] = _##precision##operation(a[15], b[15]);                             \
    return c;                                                                  \
  }

define_2x_wrapper(float, add);
define_2x_wrapper(float, sub);
define_2x_wrapper(float, mul);
define_2x_wrapper(float, div);
define_2x_wrapper(double, add);
define_2x_wrapper(double, sub);
define_2x_wrapper(double, mul);
define_2x_wrapper(double, div);

define_4x_wrapper(float, add);
define_4x_wrapper(float, sub);
define_4x_wrapper(float, mul);
define_4x_wrapper(float, div);
define_4x_wrapper(double, add);
define_4x_wrapper(double, sub);
define_4x_wrapper(double, mul);
define_4x_wrapper(double, div);

define_8x_wrapper(float, add);
define_8x_wrapper(float, sub);
define_8x_wrapper(float, mul);
define_8x_wrapper(float, div);
define_8x_wrapper(double, add);
define_8x_wrapper(double, sub);
define_8x_wrapper(double, mul);
define_8x_wrapper(double, div);

define_16x_wrapper(float, add);
define_16x_wrapper(float, sub);
define_16x_wrapper(float, mul);
define_16x_wrapper(float, div);
define_16x_wrapper(double, add);
define_16x_wrapper(double, sub);
define_16x_wrapper(double, mul);
define_16x_wrapper(double, div);

int2 _2xdoublecmp(enum FCMP_PREDICATE p, double2 a, double2 b) {
  int2 c;
  c[0] = _doublecmp(p, a[0], b[0]);
  c[1] = _doublecmp(p, a[1], b[1]);
  return c;
}

int2 _2xfloatcmp(enum FCMP_PREDICATE p, float2 a, float2 b) {
  int2 c;
  c[0] = _floatcmp(p, a[0], b[0]);
  c[1] = _floatcmp(p, a[1], b[1]);
  return c;
}

int4 _4xdoublecmp(enum FCMP_PREDICATE p, double4 a, double4 b) {
  int4 c;
  c[0] = _doublecmp(p, a[0], b[0]);
  c[1] = _doublecmp(p, a[1], b[1]);
  c[2] = _doublecmp(p, a[2], b[2]);
  c[3] = _doublecmp(p, a[3], b[3]);
  return c;
}

int4 _4xfloatcmp(enum FCMP_PREDICATE p, float4 a, float4 b) {
  int4 c;
  c[0] = _floatcmp(p, a[0], b[0]);
  c[1] = _floatcmp(p, a[1], b[1]);
  c[2] = _floatcmp(p, a[2], b[2]);
  c[3] = _floatcmp(p, a[3], b[3]);
  return c;
}

int8 _8xdoublecmp(enum FCMP_PREDICATE p, double8 a, double8 b) {
  int8 c;
  c[0] = _doublecmp(p, a[0], b[0]);
  c[1] = _doublecmp(p, a[1], b[1]);
  c[2] = _doublecmp(p, a[2], b[2]);
  c[3] = _doublecmp(p, a[3], b[3]);
  c[4] = _doublecmp(p, a[4], b[4]);
  c[5] = _doublecmp(p, a[5], b[5]);
  c[6] = _doublecmp(p, a[6], b[6]);
  c[7] = _doublecmp(p, a[7], b[7]);
  return c;
}

int8 _8xfloatcmp(enum FCMP_PREDICATE p, float8 a, float8 b) {
  int8 c;
  c[0] = _floatcmp(p, a[0], b[0]);
  c[1] = _floatcmp(p, a[1], b[1]);
  c[2] = _floatcmp(p, a[2], b[2]);
  c[3] = _floatcmp(p, a[3], b[3]);
  c[4] = _floatcmp(p, a[4], b[4]);
  c[5] = _floatcmp(p, a[5], b[5]);
  c[6] = _floatcmp(p, a[6], b[6]);
  c[7] = _floatcmp(p, a[7], b[7]);
  return c;
}

int16 _16xfloatcmp(enum FCMP_PREDICATE p, float16 a, float16 b) {
  int16 c;
  c[0] = _floatcmp(p, a[0], b[0]);
  c[1] = _floatcmp(p, a[1], b[1]);
  c[2] = _floatcmp(p, a[2], b[2]);
  c[3] = _floatcmp(p, a[3], b[3]);
  c[4] = _floatcmp(p, a[4], b[4]);
  c[5] = _floatcmp(p, a[5], b[5]);
  c[6] = _floatcmp(p, a[6], b[6]);
  c[7] = _floatcmp(p, a[7], b[7]);
  c[8] = _floatcmp(p, a[8], b[8]);
  c[9] = _floatcmp(p, a[9], b[9]);
  c[10] = _floatcmp(p, a[10], b[10]);
  c[11] = _floatcmp(p, a[11], b[11]);
  c[12] = _floatcmp(p, a[12], b[12]);
  c[13] = _floatcmp(p, a[13], b[13]);
  c[14] = _floatcmp(p, a[14], b[14]);
  c[15] = _floatcmp(p, a[15], b[15]);
  return c;
}
