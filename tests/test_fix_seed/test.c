#include <stdlib.h>
#include <stdio.h>

#define REAL double
#define N_ITER 100

int main(void) {
  int i;

  /* initialize random seed */
  srand(0);

  REAL sum = 0.0;

  /* Sum 0.1 N_ITER time */
  for (i=0; i<N_ITER; i ++) {
    sum += 0.1;
    fprintf(stderr, "%.13a\n", sum);
  }


  return 0;
}
