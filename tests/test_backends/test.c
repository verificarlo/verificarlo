#include <stdlib.h>
#include <stdio.h>

#ifndef NB_ITER
#define NB_ITER 50
#endif

#if REAL == double
#define FMT "%.13a\n"
#elif REAL == float
#define FMT "%.6a\n"
#endif

REAL operate(REAL a, REAL b) {
    return a OPERATION b;
}

static void do_test(REAL a, REAL b) {
  int i;
  for (i = 0; i < SAMPLES; i++) {
    printf(FMT, operate(a,b));
  }
  double sig = compute_sig(SAMPLES, data);

  printf("TEST>%g\n", sig);
}

int main(void) {
  int i;
  // initialize random seed
  srand(0);

  // Test with NB_ITER different random operands
  for (i=0; i<NB_ITER; i ++) {
    do_test((REAL)rand()/(REAL)RAND_MAX, (REAL)rand()/(REAL)RAND_MAX);
  }

  return 0;
}
