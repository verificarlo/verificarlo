#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#ifdef FLOAT
#define REAL float
#else
#define REAL double
#endif

/* u_{k+1} = 111 - \frac{1130}{u_k} + \frac{3000}{u_k \times u_{k-1}} */
REAL muller1(REAL u_k, REAL u_km1) {
  return 111.0 - 1130.0/u_k + 3000.0/(u_k*u_km1);
}

REAL muller2(REAL x) {
  return (3.0*x*x*x*x - 20.0*x*x*x + 35.0*x*x - 24.0 ) / (4.0*x*x*x - 30.0*x*x + 70.0* x - 50.0);
}

int main(int argc, char *argv[]) {

  int i = 0;

  REAL u_k = 2.0, u_kp1 = -4.0, t = 0;
  for (i = 0; i < NITER; i++) {
    t = u_kp1;
    u_kp1 = muller1(u_kp1, u_k);
    u_k = t;
  }
  printf("%.16e\n",u_kp1);

  REAL x = 1.5100050721319;
  
  for (i = 0; i < NITER; i++) {
    x = muller2(x);
  }
  /* printf("%.16e\n",x); */
  
  return 0;
}
