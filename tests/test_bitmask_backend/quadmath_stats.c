#include <assert.h>
#include <math.h>
#include <quadmath.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpfr.h>

#ifndef DEBUG
#define DEBUG 0
#endif

/* Computes the means */
__float128 compute_mean(const int N, const __float128 data[]) {
  assert(N >= 1);

  __float128 sum = data[0];
  __float128 c = 0.0, y, t;
  int i;

  for (i = 1; i < N; i++) {
    y = data[i] - c;
    t = sum + y;
    c = (t - sum) - y;
    sum = t;
  }

  return sum / (__float128)N;
}

__float128 compute_variance(const int N, const __float128 mean,
                            const __float128 data[]) {
  /* Knuth compensated online variance */
  assert(N >= 2);

  int i;
  int n = 0;
  __float128 m = mean, M2 = 0.0;

  for (i = 0; i < N; i++) {
    n++;
    __float128 delta = data[i] - m;
    m += delta / (__float128)n;
    M2 += delta * (data[i] - m);
  }

  return M2 / (__float128)(n - 1);
}

/* Computes the number of significant digits */
double compute_sig(const int N, const __float128 data[]) {
  __float128 mean = compute_mean(N, data);
  __float128 std = sqrtq(compute_variance(N, mean, data));

  if (DEBUG) {
    char mean_str[1024], std_str[1024];
    quadmath_snprintf(mean_str, sizeof(mean_str), "%.29Qa", mean);
    quadmath_snprintf(std_str, sizeof(std_str), "%.29Qa", std);
    fprintf(stderr, "mean %s, std %s\n", mean_str, std_str);

    quadmath_snprintf(mean_str, sizeof(mean_str), "%.29QE", mean);
    quadmath_snprintf(std_str, sizeof(std_str), "%.29QE", std);
    fprintf(stderr, "mean %s, std %s\n", mean_str, std_str);
  }

  return (mean == 0.0q && std == 0.0q) ? 0 : -log2q(fabsq(std / mean));
}

int main(int argc, char *argv[]) {

  if (argc != 3) {
    fprintf(stderr, "Usage: <N> <filename>\n");
    exit(EXIT_FAILURE);   
  }

  __float128 data[SAMPLES];

  const int N = atoi(argv[1]);
  FILE *fi = fopen(argv[2], "r");
  char *line = NULL;
  size_t len = 0;
  ssize_t nread;

  
  int i = 0;
  
  if (fi != NULL) {
    while ((nread = getline(&line, &len, fi)  != -1) && i < N) {
      data[i] = strtoflt128(line ,NULL);
      i++;
    }
  }

  assert(i==N);
  
  double s = compute_sig(N, data);

  printf("%g\n", s);
}
