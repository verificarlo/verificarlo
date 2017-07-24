#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
#include <stdint.h>

#include "bitmask_const.h"

int main(void) {
  REAL a = 0.0, b = 0.0, res = 0.0;
  INT_T *tmp = (INT_T*)&a;
  *tmp = REAL_TEST_VALUE;
  res = a + b;
  printf("%a\n", NEAREST_REAL(res));
  return 0;
}
