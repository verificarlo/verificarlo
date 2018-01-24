#include <stdio.h>
#include <math.h>
#include <assert.h>

#define REAL double

__attribute__ ((noinline))  void solve(const REAL A[4], const REAL B[2], REAL C[2])
{
  REAL det = A[0] * A[3] - A[2] * A[1];

  REAL detX = B[0] * A[3] - B[1] * A[1];
  REAL detY = A[0] * B[1] - A[2] * B[0];

  C[0] = detX/det;
  C[1] = detY/det;
}

int main(int argc,char *argv[])
{
    REAL A[4] = {.2161, .1441, 1.2969, .8648};
    REAL B[2] = {.1440, .8642};

    REAL C[2] = {0,0};

    solve(A, B, C);

    /* Print solution */
    printf("%.16e %.16e\n", C[0], C[1]);
    return 0;
}
