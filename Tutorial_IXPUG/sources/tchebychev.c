/* Verificarlo Tutorial: Tchebychev Polynomial Evaluation */

#include<assert.h>
#include<stdlib.h>
#include<stdio.h>
#include<float.h>
#include<string.h>

/* Define real type and format string */
#ifdef DOUBLE
#define REAL double
#define FMT "%.16e %.16e"
#else
#define REAL float
#define FMT "%.7e %.7e"
#endif

/* Coefficients of the Tchebychev polynomial */
REAL a[] = {
  1.,
  - 200.,
  6600.,
  - 84480.,
  549120.,
  - 2050048.,
  4659200.,
  - 6553600.,
  5570560.,
  -2621440.,
  524288.
};

/* Expanded naïve implementation of the polynomial evaluation */
/* D. Stott Parker, MCA, section 8.1.2 pp.52-54               */
REAL expanded(REAL x) {
  REAL r = a[0];
  REAL x2 = x*x;
  REAL p = x2;

  for (int i = 1; i <= 10; i++) {
    r += a[i]*p;
    p *= x2;
  }

  return r;
}

/* Factored polynomial evaluation */
REAL factored(REAL x) {
  REAL r = 0.0;
  REAL x2 = x*x;

  r = 8.0*x2;
  r *= (x - 1.0);
  r *= (x + 1.0);
  r *= (4.0*x2 + 2.0*x - 1.0)*(4.0*x2 + 2.0*x - 1.0);
  r *= (4.0*x2 - 2.0*x - 1.0)*(4.0*x2 - 2.0*x - 1.0);
  r *= (16.0*x2*x2 - 20.0*x2 + 5.0)*(16.0*x2*x2 - 20.0*x2 + 5.0);
  r += 1.0;

  return r;
}

/* Horner polynomial evaluation */
REAL horner(REAL x) {
  REAL r = a[10];
  REAL x2 = x*x;

  for (int i = 9; i >= 0; i--) {
    r = r*x2 + a[i];
  }
  return r;
}

/* usage: fails with usage message */
void usage(void) {
  fprintf(stderr, "usage: tchebychev x [EXPANDED | HORNER | FACTORED]\n"
                  "       x is a floating point value\n");
  exit(EXIT_FAILURE);
}

/* Main function */
int main (int argc, char ** argv)
{
  if (argc != 3) usage();
  REAL x = atof(argv[1]);
  char *method = argv[2];

  REAL r;

  if (strcmp(method, "EXPANDED") == 0)
    r = expanded(x);
  else if (strcmp(method, "HORNER") == 0)
    r = horner(x);
  else if (strcmp(method, "FACTORED") == 0)
    r = factored(x);
  else
    usage();

  printf(FMT"\n", x, r);
  return EXIT_SUCCESS;
}
