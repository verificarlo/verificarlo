#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#ifndef NB_ITER
#define NB_ITER 50
#endif

typedef union {
  uint32_t u32[2];
  uint64_t u64;
  double f64;
} binary64;

double get_rand_double() {
  binary64 b64;
  b64.u32[0] = rand();
  b64.u32[1] = rand();
  return b64.f64;
}

float get_rand_float() {
  int r = rand();
  return *(float*)&r;
}

#define FMT(X) _Generic((X), double : "%.13a", float: "%.6a")
#define GET_RAND(X) _Generic((X), double: get_rand_double, float: get_rand_float)

REAL operate(REAL a, REAL b) {
  return a OPERATION b;
}

REAL get_rand() {
  typeof(REAL) x;
  return GET_RAND(x)();
}

#define QUOTE(name) #name
#define STR(macro) QUOTE(macro)
#define STR_OPERATION STR(OPERATION)

static void do_test(char *fmt, REAL a, REAL b) {
  for (int i = 0; i < SAMPLES; i++) {
    printf(fmt, a, b, operate(a,b));
  }
}

int main(void) {
  /* initialize random seed */
  srand(0);

  REAL x;
  char *flt_fmt = FMT(x);
  char fmt[1024];
  sprintf(fmt, "%s %s %s = %s\n", flt_fmt, STR_OPERATION, flt_fmt, flt_fmt);
  /* Test with NB_ITER different random operands */
  for (int i=0; i<NB_ITER; i ++) {
    do_test(fmt, get_rand(), get_rand());
  }

  return 0;
}
