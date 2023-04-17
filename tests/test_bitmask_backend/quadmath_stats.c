#include "quadmath_stats.h"
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


#ifndef DEBUG
#define DEBUG 0
#endif

/* Computes the means */
FP128 compute_mean(const int N, const FP128 data[]) {
  assert(N >= 1);

  FP128 sum = data[0];
  FP128 c = 0.0, y, t;
  int i;

  for (i = 1; i < N; i++) {
    y = data[i] - c;
    t = sum + y;
    c = (t - sum) - y;
    sum = t;
  }

  return sum / (FP128)N;
}

FP128 compute_variance(const int N, const FP128 mean,
                            const FP128 data[]) {
  /* Knuth compensated online variance */
  assert(N >= 2);

  int i;
  int n = 0;
  FP128 m = mean, M2 = 0.0;

  for (i = 0; i < N; i++) {
    n++;
    FP128 delta = data[i] - m;
    m += delta / (FP128)n;
    M2 += delta * (data[i] - m);
  }

  return M2 / (FP128)(n - 1);
}

/* Computes the number of significant digits */
double compute_sig(const int N, const FP128 data[]) {
  FP128 mean = compute_mean(N, data);
  FP128 std = sqrtFP128(compute_variance(N, mean, data));

  return (mean == 0.0q && std == 0.0q) ? 0 : -log2FP128(fabsFP128(std / mean));
}

int main(int argc, char *argv[]) {

  if (argc != 3) {
    fprintf(stderr, "Usage: <N> <filename>\n");
    exit(EXIT_FAILURE);
  }

  FP128 data[SAMPLES];

  const int N = atoi(argv[1]);
  FILE *fi = fopen(argv[2], "r");
  char *line = NULL;
  size_t len = 0;
  ssize_t nread;

  int i = 0;

  if (fi != NULL) {
    while ((nread = getline(&line, &len, fi) != -1) && i < N) {
      data[i] = strtoFP128(line, NULL);
      i++;
    }
  }

  assert(i == N);

  double s = compute_sig(N, data);

  printf("%g\n", s);
}
