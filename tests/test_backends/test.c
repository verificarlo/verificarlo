#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#ifndef NB_ITER
#define NB_ITER 50
#endif

#if REAL == double
#define FMT "%.13a"
#elif REAL == float
#define FMT "%.6a"
#endif

typedef union {
  uint32_t u32[2];
  uint64_t u64;
  double f64;
} binary64;

REAL operate(REAL a, REAL b) {
    return a OPERATION b;
}

REAL get_rand() {
#if REAL == double
  binary64 b64;
  b64.u32[0] = rand();
  b64.u32[1] = rand();
  return b64.f64;
#elif REAL == float
  int r = rand();
  return *(float*)&r;
#endif
}

#define QUOTE(name) #name
#define STR(macro) QUOTE(macro)
#define STR_OPERATION STR(OPERATION)

static void do_test(REAL a, REAL b) {
  int i;
  for (i = 0; i < SAMPLES; i++) {
    printf(FMT STR_OPERATION  FMT " = " FMT "\n", a, b, operate(a,b));
  }
}

int main(void) {
  int i;
  /* initialize random seed */
  srand(0);

  /* Test with NB_ITER different random operands */
  for (i=0; i<NB_ITER; i ++) {
    do_test(get_rand(), get_rand());
  }

  return 0;
}
