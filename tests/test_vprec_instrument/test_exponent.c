#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define bit(A, i) ((A >> i) & 1)

typedef unsigned long long int u_64;
typedef unsigned int u_32;

static void print_double(double d) {
  u_64 a = *((u_64 *)&d);

  printf("\t\t");
  for (int i = 63; i >= 0; i--)
    (i == 63 || i == 52) ? printf("%lld ", bit(a, i))
                         : printf("%lld", bit(a, i));
}

static void print_float(float f) {
  u_32 a = *((u_32 *)&f);

  printf("\t\t");
  for (int i = 31; i >= 0; i--)
    (i == 31 || i == 23) ? printf("%d ", bit(a, i)) : printf("%d", bit(a, i));
}

static void print_log(double *d, float *f, char *log) {
  if (d != NULL){
    print_double(*d);
  }else if (f != NULL){
    print_float(*f);
  }else{
    fprintf(stderr, "undefined real size\n");
  }

  printf("%s\n", log);
}

static union {
  u_32 i;
  float f;
} t_32;
static union {
  u_64 i;
  double f;
} t_64;

double Fdouble(double a)
{
  //print_double(a);
  print_log(&a, NULL, " <= Input");

  double zero = 0.0;
  double c = a + zero;

  //print_double(c);
  print_log(&c, NULL, " <= Internal Operation");

  return t_64.f;
}

float Ffloat(float a)
{
  //print_float(a);
  print_log(NULL, &a, " <= Input");

  float zero = 0.0;
  float c = a + zero;

  //print_float(c);
  print_log(NULL, &c, " <= Internal Operation");

  return t_32.f;
}

int main(int argc, char const *argv[]) {

  t_64.i = 0x0FFFFFFFFFFFFFFF;
  t_32.i = 0x0FFFFFFF;

  float f = Ffloat(t_32.f);

  //print_float(f);
  print_log(NULL, &f, " <= Output");

  double d = Fdouble(t_64.f);

  //print_double(d);
  print_log(&d, NULL, " <= Output");

  return 0;
}