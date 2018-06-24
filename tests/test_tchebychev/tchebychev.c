#include<assert.h>
#include<stdlib.h>
#include<stdio.h>
#include<float.h>
#include<string.h>

#ifdef DOUBLE
#define REAL double
#define FMT "%.16e %.16e"
#else
#define REAL float
#define FMT "%.7e %.7e"
#endif

REAL coefs[] = {
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

/* DS Parker, MCA, section 8.1.2 pp.52-54 */

REAL expanded(REAL z) {
  REAL r = coefs[0];
  REAL z2 = z*z;
  REAL p = z2;

  for (int i = 1; i <= 10; i++) {
    r += coefs[i]*p;
    p *= z2;
  }
  
  return r;
}

REAL factored(REAL z) {
  REAL r = 0.0;
  REAL z2 = z*z;

  r = 8.0*z2;
  r *= (z - 1.0);
  r *= (z + 1.0);
  r *= (4.0*z2 + 2.0*z - 1.0)*(4.0*z2 + 2.0*z - 1.0);
  r *= (4.0*z2 - 2.0*z - 1.0)*(4.0*z2 - 2.0*z - 1.0);
  r *= (16.0*z2*z2 - 20.0*z2 + 5.0)*(16.0*z2*z2 - 20.0*z2 + 5.0);
  r += 1.0;
    
  return r;
}

REAL horner(REAL z) {  
  REAL r = coefs[10];
  REAL z2 = z*z;
  
  for (int i = 9; i >= 0; i--) {
    r = r*z2 + coefs[i];
  }
  
  return r;
}

int main (int argc, char ** argv)
{
  assert(argc == 3);
  REAL z = atof(argv[1]);
  char *method = argv[2];

  REAL r;
  
  if (strcmp(method, "EXPANDED") == 0)
    r = expanded(z);
  else if (strcmp(method, "HORNER") == 0)
    r = horner(z);
  else if (strcmp(method, "FACTORED") == 0)
    r = factored(z);
  else    
    assert(0);
  
  printf(FMT"\n", z, r);    
}
