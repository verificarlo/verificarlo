#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define bit(A, i) ((A >> i) & 1)

typedef unsigned long long int u_64;
typedef unsigned int u_32;

static void print_double(double d) {
  u_64 a = *((u_64 *)&d);

  for (int i = 63; i >= 0; i--)
    (i == 63 || i == 52) ? printf("%lld ", bit(a, i))
                         : printf("%lld", bit(a, i));

  printf("\n");
}

static void print_float(float f) {
  u_32 a = *((u_32 *)&f);

  for (int i = 31; i >= 0; i--)
    (i == 31 || i == 23) ? printf("%d ", bit(a, i)) : printf("%d", bit(a, i));

  printf("\n");
}

static union {
  u_32 i;
  float f;
} t_32;
static union {
  u_64 i;
  double f;
} t_64;

double Fdouble(double a, double b) {
  double zero = 0.0;

  print_double(a);
  print_double(b);

  double c = a + zero;
  print_double(c);

  print_double(pow(a, 1));

  double d = b + zero;
  print_double(d);

  return t_64.f;
}

float Ffloat(float a, float b) {
  float zero = 0.0;

  print_float(a);
  print_float(b);

  float c = a + zero;
  print_float(c);

  print_float(powf(a, 1));

  float d = b + zero;
  print_float(d);

  return t_32.f;
}

int main(int argc, char const *argv[]) {
  if (argc < 3) {
    fprintf(stderr, "./test [double mantissa_size] [float mantissa_size]\n");
    abort();
  }

  int size_64 = atoi(argv[1]);
  int size_32 = atoi(argv[2]);

  t_64.f = 1.0;
  t_32.f = 1.0;

  t_64.i |= (1 << (52-size_64)) - 1;
  t_32.i |= (1 << (23-size_32)) - 1;;

  print_double(Fdouble(t_64.f, t_64.f));
  print_float(Ffloat(t_32.f, t_32.f));

  return 0;
}
