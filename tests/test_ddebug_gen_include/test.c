#include<stdio.h>

__attribute__ ((noinline)) double compute(double a, double b) {
  return (a-b) + (a*b) + (a/b);
}

int main(void) {
  double a = 1.2345678e-5;
  double b = 9.8765432e12;
  double c = compute(a,b);
  double ref = (a-b) + (a*b) + (a/b);
  if (c == ref) {
    printf("result is correct %g == %g (ref)\n", c, ref);
    return 0;
  } else {
    printf("result is not correct %g != %g (ref)\n", c, ref);
    return 1;
  }
}
