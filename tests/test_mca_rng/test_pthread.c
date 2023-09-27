#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/sysinfo.h>
#include <unistd.h>

#include "interflop/common/float_const.h"

#define FMT(X) _Generic((X), double : "%.16a", float : "%.7a")
#define N 12

typedef struct {
  REAL a;
  REAL b;
  char *fmt;
} work_args_t;

void *do_work(void *arg) {
  REAL res;
  work_args_t *func_args = (work_args_t *)arg;

  res = func_args->a + func_args->b;

  printf(func_args->fmt, func_args->a, func_args->b, res, getppid(),
         syscall(__NR_gettid));

  return NULL;
}

int main(void) {
  REAL x;
  char *flt_fmt = FMT(x);
  char fmt[1024];
  int i;
  pthread_t tid[N];

  sprintf(fmt, "a=%s, b=%s, a+b=%s, pid=%%d, tid=%%d\n", flt_fmt, flt_fmt,
          flt_fmt);

  for (i = 0; i < N; i++) {
    REAL a, b;
    int err;
    work_args_t *args_tmp = (work_args_t *)malloc(sizeof(work_args_t));

    a = 0.1;
    b = 0.1;

    args_tmp->a = a;
    args_tmp->b = b;
    args_tmp->fmt = fmt;
    err = pthread_create(&(tid[i]), NULL, &do_work, (void *)args_tmp);
    if (err != 0) {
      printf("\ncan't create thread :[%s]", strerror(err));
      exit(EXIT_FAILURE);
    }
  }

  for (i = 0; i < N; i++) {
    pthread_join(tid[i], NULL);
  }

  return 0;
}
