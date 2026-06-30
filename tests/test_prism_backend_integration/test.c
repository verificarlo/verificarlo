/* Tests for the PRISM interflop backend (libinterflop_prism.so).
 *
 * This binary links vfcwrapper (for VFC_BACKENDS / interflop_call support)
 * and libprism-dynamic (for the precision getter symbols).  The test is
 * intentionally not PRISM-instrumented: we only verify that the backend
 * correctly sets and changes the PRISM virtual precision.
 *
 * Usage:
 *   test 0   – print precision values set at startup via VFC_BACKENDS
 *   test 1   – change precision via interflop_call, then print before/after
 */

#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "interflop/interflop.h"

typedef int32_t (*get_prec_t)(void);

static get_prec_t lookup_getter(const char *sym) {
  dlerror();
  get_prec_t fn = (get_prec_t)dlsym(RTLD_DEFAULT, sym);
  const char *err = dlerror();
  if (fn == NULL) {
    fprintf(stderr, "Error: PRISM getter '%s' not found: %s\n", sym,
            err ? err : "symbol not found");
    exit(1);
  }
  return fn;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <0|1>\n", argv[0]);
    return 1;
  }

  get_prec_t get_f32 =
      lookup_getter("interflop_prism_get_default_virtual_precision_binary32");
  get_prec_t get_f64 =
      lookup_getter("interflop_prism_get_default_virtual_precision_binary64");

  int test_case = atoi(argv[1]);

  if (test_case == 0) {
    /* Report the precision set at startup via VFC_BACKENDS */
    get_prec_t get_round_mode = lookup_getter("interflop_prism_get_rounding_mode");
    printf("precision_binary32=%d\n", get_f32());
    printf("precision_binary64=%d\n", get_f64());
    printf("mode=%d\n", get_round_mode());

  } else if (test_case == 1) {
    /* Test that interflop_call changes virtual precision at runtime */
    get_prec_t get_round_mode = lookup_getter("interflop_prism_get_rounding_mode");

    int32_t before_f32 = get_f32();
    int32_t before_f64 = get_f64();
    int32_t before_mode = get_round_mode();

    interflop_call(INTERFLOP_SET_PRECISION_BINARY32, 5);
    interflop_call(INTERFLOP_SET_PRECISION_BINARY64, 10);
    interflop_call(INTERFLOP_SET_ROUNDING_MODE, 1);

    printf("before_f32=%d after_f32=%d\n", before_f32, get_f32());
    printf("before_f64=%d after_f64=%d\n", before_f64, get_f64());
    printf("before_mode=%d after_mode=%d\n", before_mode, get_round_mode());

  } else {
    fprintf(stderr, "Unknown test case %d\n", test_case);
    return 1;
  }

  return 0;
}
