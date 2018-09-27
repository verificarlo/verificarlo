/* Verificarlo Tutorial: Tchebychev Polynomial Evaluation */

#include<assert.h>
#include<stdlib.h>
#include<stdio.h>
#include<float.h>
#include<string.h>

#include "libeft.h"

/* Define real type and format string */
#ifdef DOUBLE
#define REAL double
#define FMT "%.16e %.16e"
#define TWOPROD twoprod_d
#define TWOSUM  twosum_d
#else
#define REAL float
#define FMT "%.7e %.7e"
#define TWOPROD twoprod_s
#define TWOSUM  twosum_s
#endif

void my_split_s(float a, float *x, float *y){
  const float c = 4097 * a;
  (*x) = c - (c-a);
  (*y) = a - (*x);
}

void my_twosum_s (float a, float b,
                float *x, float *e) {
  (*x) = a + b;
  const float z = (*x) - a;
  (*e) = (a - ((*x)-z)) + (b-z);
}

void my_twoprod_s (float a, float b,
                 float *x, float *e) {
  (*x) = a * b;

  float a1, a2, b1, b2;
  my_split_s (a, &a1, &a2);
  my_split_s (b, &b1, &b2);

  const float tmp = (a1*b1-(*x)) + a1*b2 + a2*b1;
  (*e) = tmp + a2*b2;
}


void my_split_d(double a, double *x, double *y){
  const double c = 134217729 * a;
  (*x) = c - (c-a);
  (*y) = a - (*x);
}

void my_twosum_d (double a, double b,
                double *x, double *e) {
  (*x) = a + b;
  const double z = (*x) - a;
  (*e) = (a - ((*x)-z)) + (b-z);
}

void my_twoprod_d (double a, double b,
                 double *x, double *e) {
  (*x) = a * b;

  double a1, a2, b1, b2;
  my_split_d (a, &a1, &a2);
  my_split_d (b, &b1, &b2);

  const double tmp = (a1*b1-(*x)) + a1*b2 + a2*b1;
  (*e) = tmp + a2*b2;
}



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

/* Compensated Horner polynomial evaluation */
REAL compHorner(REAL x) {
  REAL r = a[10];
  REAL s = 0;
  REAL x2 = x*x;

  for (int i = 9 ; i >= 0 ; i--) {
    REAL p, pe, se;
    TWOPROD(r, x2, &p, &pe);  // (p+pe) = r*x2
    TWOSUM(p, a[i], &r, &se); // (r+se) = p + a[i]
    s = s*x2+(pe+se);
  }

  return r+s;
}

/* usage: fails with usage message */
void usage(void) {
  fprintf(stderr, "usage: tchebychev x [EXPANDED | HORNER | FACTORED | COMPHORNER]\n"
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
  else if (strcmp(method, "COMPHORNER") == 0)
    r = compHorner(x);
  else
    usage();

  printf(FMT"\n", x, r);
  return EXIT_SUCCESS;
}
