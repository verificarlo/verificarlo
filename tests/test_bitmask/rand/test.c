#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#define N 100
#define SAMPLE 1000

#ifndef FLOAT
#define REAL double
#else
#define REAL float
#endif

REAL get_random() {
  return rand()/(REAL)RAND_MAX; 
}

REAL operate(REAL a, REAL b) {
  return a OPERATION b;
}

int main(int argc, char * argv[]) {

  // Fix seed
  srand(1);

  REAL a = 0.0, b = 0.0, res = 0.0;

  int i,j;
  for (i = 0; i < N; i++) {
    a = get_random();
    b = get_random();
    for (j = 0; j < SAMPLE; j++) {
      res = operate(a,b);
      printf("%.16e ", res);
    }
    printf("\n");
  }
  
  return 0;
}
