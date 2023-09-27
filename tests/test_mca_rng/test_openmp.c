#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <sys/sysinfo.h>
#include <unistd.h>

#include "interflop/common/float_const.h"

#define FMT(X) _Generic((X), double : "%.16e", float : "%.7e")
#define N 12

int main(void) {
  REAL x;
  char *flt_fmt = FMT(x);
  char fmt[1024];
  int i;

  sprintf(fmt, "a=%s, b=%s, a+b=%s, pid=%%d, tid=%%d\n", flt_fmt, flt_fmt,
          flt_fmt);

#pragma omp parallel
#pragma omp for
  for (i = 0; i < N; i++) {
    // do something with i
    REAL a, b, res;

    a = 0.1;
    b = 0.1;

    res = a + b;

    printf(fmt, a, b, res, getppid(), syscall(__NR_gettid));
  }

  return 0;
}
