#include <stdio.h>

typedef float float4 __attribute__((ext_vector_type(4)));
typedef double double2 __attribute__((ext_vector_type(2)));

#define A(X) _Generic(X, float4: 0.1f, double2: 0.1)
#define B(X) _Generic(X, float4: 0.3f, double2: 0.3)
#define SIZE(X) _Generic(X, float4: 4, double2: 2)

int main(void) {

  typeof(REAL) a,b,c;
  a = A(a);
  b = B(b);
  c = a + b;

  int size = SIZE(a);
  int ret = 0;

  for (int i = 0; i < size; i++) {
    printf("%f + %f = %f\n", a[i], b[i], c[i]);
  }

  return 0;
}

