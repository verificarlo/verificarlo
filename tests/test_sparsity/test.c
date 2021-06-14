#include <stdlib.h>
#include <stdio.h>

#define REAL double
#define N_ITER 1000

int main(void) {
  int i;

  /* initialize random seed */
  srand(0);

  REAL sum = 0.0;

  /* Sum 0.1 N_ITER time */
  for (i=0; i<N_ITER; i ++) {
    sum += 0.1;
  }


  return 0;
}
