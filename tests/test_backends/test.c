#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <float.h>

REAL data[SAMPLES];

static double compute_mean(int N, REAL data[]) {
  assert (N >= 1);

  double sum = data[0];
  double c = 0.0,y,t;
  int i;

  for (i=1; i<N; i++) {
    y = data[i] - c;
    t = sum + y;
    c = (t - sum) - y;
    sum = t;
  }

  return sum/(double)N;
}

static double compute_variance(int N, double mean, REAL data[]) {
  /* Knuth compensated online variance */
  assert(N >= 2);

  int i;
  int n = 0;
  double m = mean, M2 = 0.0;

  for (i=0; i<N; i++) {
    n ++;
    double delta = data[i] - m;
    m += delta/(double)n;
    M2 += delta*(data[i] - m);
  }

  return M2 / (double)(n - 1);
}

static double compute_sig(int N, REAL data[]) {
  double mean = compute_mean(N, data);
  double stdev = sqrt(compute_variance(N, mean, data));
  return -log2(stdev/mean);
}

REAL operate(REAL a, REAL b) {
    return a OPERATION b;
}

static void do_test(REAL a, REAL b) {
  int i;
  for (i = 0; i < SAMPLES; i++) {
    //printf("expecting a+b=%.17g\n", a+b);
    data[i] = operate(a,b);
    //printf("noisy a+b=%.17g\n", data[i]);
  }
  double sig = compute_sig(SAMPLES, data);

  printf("TEST>%g\n", sig);
}

int main(void) {
  int i;
  // initialize random seed
  srand(0);

  // Test with 50 different random operands
  for (i=0; i<50; i ++) {
    do_test((REAL)rand()/(REAL)RAND_MAX, (REAL)rand()/(REAL)RAND_MAX);
  }

  // Test with extreme values
  //do_test(DBL_MIN,DBL_MAX);
  //do_test(DBL_MIN,DBL_MIN);
  //do_test(DBL_MAX,DBL_MAX);
  return 0;
}
