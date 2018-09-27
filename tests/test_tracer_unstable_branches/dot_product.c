#include <stdlib.h>
#include <stdio.h>
#include <math.h>
/* #include "libeft.h" */

#ifdef FLOAT
#define REAL float
#define SQRTF sqrtf
#define REAL_FMT "%+.6a\n"
#define POW powf
#else
#define REAL double
#define SQRTF sqrt
#define REAL_FMT "%+.13a\n"
#define POW pow
#endif

#ifndef ITER
#define ITER 1000
#endif

REAL SQRT(REAL x) {
  if (x < 0.0) {
    fprintf(stderr, "Error while computing the square root of a negative number "REAL_FMT, x);
    exit(1);
  }
  return SQRTF(x);
}

__attribute__ ((noinline)) REAL randf(REAL a, REAL b) {
  /* real in [0,1] */
  REAL r = (REAL)rand()/(REAL)RAND_MAX;
  return (b-a)*r + a;
}

void shuffle_vector(int n, REAL x[n]) {
  REAL tmp;
  for (int i = 0; i < n; i++) {
    int j = rand() % n;
    tmp = x[i];
    x[i] = x[j];
    x[j] = tmp;
  }
}

void print_vector(int n, REAL x[n], char *msg) {
  printf("--%s--\n",msg);
  for (int i = 0; i < n; i++)
    printf("%+.17e\n",x[i]);
  printf("-----\n");
}

REAL naive_dot_product(int n, REAL x[n], REAL y[n]) {
  REAL res = 0.0;
  for (int i = 0; i < n; i++)
    res += x[i]*y[i];
  return res;
}

void gen_ill_dot(int n, REAL x[n], REAL y[n]) {
  for (int i = 0; i < n; i++) {
    x[i] = 0.1 * i;
    y[i] = 0.01 * i * i;
  }   
}

void gen_well_dot(int n, REAL x[n], REAL y[n]){
  for (int i = 0; i < n; i++) {
    x[i] = randf(0,1);
    y[i] = randf(0,1);
  }  
}

void gen_zero_dot(int n, REAL x[n], REAL y[n]){
  for (int i = 0; i < n; i++) {
    x[i] = randf(-1,1);
    y[i] = POW(-1,i)*(1/(x[i]));
  }
}

void init(int n, REAL x[n], REAL init) {
  for (int i = 0; i < n; i++)
    x[i] = init;
}

void dot_zero(int n, REAL x[n], REAL y[n]) {
  REAL dot_naive, dot_accurate;
  gen_zero_dot(n,x,y);
  dot_naive = naive_dot_product(n,x,y);
}

void dot_well(int n, REAL x[n], REAL y[n]) {
  REAL dot_naive, dot_accurate;
  gen_well_dot(n,x,y);
  dot_naive = naive_dot_product(n,x,y);
}

int main(int argc, char * argv[]) {

  if (argc != 2) {
    fprintf(stderr, "1 arg expected\n");
    return 1;
  }
  
  int n = atoi(argv[1]);
  REAL c = 10;
  REAL x[n],y[n];
  init(n,x,0);
  init(n,y,0);
  
  for (int i = 0 ; i < 50; i++) {
    if (rand() % 2 == 0)
      dot_zero(n,x,y);
    else
      dot_well(n,x,y);
  }
  return 0;
}
