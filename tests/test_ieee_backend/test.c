#include<stdio.h>

#if __SSE__

typedef double double2 __attribute__((ext_vector_type(2)));

__attribute__ ((noinline)) double2 compute_vector(double2 a, double2 b) {
  return (a-b) + (a*b) + (a/b);
}

#endif

__attribute__ ((noinline)) double compute(double a, double b) {
  return (a-b) + (a*b) + (a/b);
}

int main(void) {
  int ret = 0;
  
  printf("double\n");
  double a = 1.2345678e-5;
  double b = 9.8765432e12;
  double c = compute(a,b);
  double ref = (a-b) + (a*b) + (a/b);
  if (c != ref) {
    printf("result is not correct %g != %g (ref)\n", c, ref);
    ret = 1;
  } else {
    printf("result is correct %g == %g (ref)\n", c, ref);
  }

#if __SSE__
  
  printf("double2\n");
  double2 v_a = {1.2345678e-5, 2.3456789e6  };
  double2 v_b = {9.8765432e12, 8.7654321e-13};
  double2 v_c = compute_vector(v_a,v_b);
  double2 v_ref = (v_a-v_b) + (v_a*v_b) + (v_a/v_b);

  if (c != ref) {
    for (int i = 0; i < 2; i++) {
      printf("result is not correct %g != %g (ref)\n", v_c[i], v_ref[i]);
    }
    ret = 1;
  } else {
    for (int i = 0; i < 2; i++) {
      printf("result is correct %g == %g (ref)\n", v_c[i], v_ref[i]);
    }
  }

#endif

  return ret;
}
