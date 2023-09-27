#include <assert.h>
#include <float.h>
#include <interflop/interflop.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
  assert(argc == 3);

  int change_prec = atoi(argv[1]);
  int change_range = atoi(argv[2]);

  if (change_prec)
    interflop_call(INTERFLOP_SET_PRECISION_BINARY32, 10);

  if (change_range)
    interflop_call(INTERFLOP_SET_RANGE_BINARY32, 2);

  float r = 3.25938657906;
  float p = 4.05896796763;

  for (int i = 0; i < 5; i++)
    r = r * p;

  fprintf(stdout, "%f\n", r);
}
