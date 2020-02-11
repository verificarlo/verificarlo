#include <stdio.h>

#define A(X) _Generic(X, float: 0.1f, double: 0.1)
#define B(X) _Generic(X, float: 0.3f, double: 0.3)

int main(void) {

  typeof(REAL) a,b,c;
  a = A(a);
  b = B(b);
  c = a + b;

  printf("%f + %f = %f\n", a, b, c);

  return 0;
}
