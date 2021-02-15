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

double Fdouble(double a, double b) {
  double zero = 0.0;

  /*
  printf("Input Argument :\t\t\t\t\t");
  print_double(a);
  */
  print_log(&a, NULL, " <= Input");

  /*
  printf("Input Argument :\t\t\t\t\t");
  print_double(b);
  */
  print_log(&b, NULL, " <= Input");

  double c = a + zero;
  print_log(&c, NULL, " <= Internal Operation");
  /*
  printf("Internal Operation :\t\t\t");
  print_double(c);
  */

  double e = pow(a,1);
  print_log(&e, NULL, " <= Internal Function Call");
  /*
  printf("Internal Function Call :\t");
  print_double(pow(a, 1));
  */

  double d = b + zero;
  /*
  printf("Internal Operation :\t\t\t");
  print_double(d);
  */
  print_log(&d, NULL, " <= Internal Operation");

  return t_64.f;
}

float Ffloat(float a, float b) {
  float zero = 0.0;

  /*
  printf("Input Argument :\t\t\t\t\t");
  print_float(a);
  */
  print_log(NULL, &a, " <= Input");
  /*
  printf("Input Argument :\t\t\t\t\t");
  print_float(b);
  */
  print_log(NULL, &b, " <= Input");

  float c = a + zero;
  print_log(NULL, &c, " <= Internal Operation");  
  /*
  printf("Internal Operation :\t\t\t");
  print_float(c);
  */

  float e = powf(a,1);
  print_log(NULL, &e, " <= Internal Function Call");
  /*
  printf("Internal Function Call :\t");
  print_float(powf(a, 1));
  */

  float d = b + zero;
  /*
  printf("Internal Operation :\t\t\t");
  print_float(d);
  */
  print_log(NULL, &d, " <= Internal Operation");


  return t_32.f;
}

int main(int argc, char const *argv[]) {
  if (argc < 3) {
    fprintf(stderr, "./test_mantissa [double mantissa_size] [float mantissa_size]\n");
    abort();
  }

  int size_64 = atoi(argv[1]);
  int size_32 = atoi(argv[2]);


  if (size_64 != -1) {
    t_64.i = pow(2, 52 - size_64) - 1;

    double out1 = Fdouble(t_64.f, t_64.f);
    print_log(&out1, NULL, " <= Output");
    /*
    printf("Output Argument :\t\t\t\t\t");
    print_double(out1);
    */
  }

  if (size_32 != -1) {
    t_32.i = pow(2, 23 - size_32) - 1;

    float out2 = Ffloat(t_32.f, t_32.f);
    print_log(NULL, &out2, " <= Output");
    /*
    printf("Output Argument :\t\t\t\t\t");
    print_float(out2);
    */
  }

  return 0;
}
