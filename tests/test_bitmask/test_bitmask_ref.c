#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
#include <stdint.h>

#include "bitmask_const.h"

void generate_real_masked(REAL *a) {
  FILE * file_ref = fopen(foutput, "w");

  int i;
  REAL cpa = (*a);
  INT_T *tmp = (INT_T*)(&cpa), mask = 0;

  for (i = 1; i <= MANTISSA_SIZE; i++) {
    cpa = *a;
    mask = MINUS_ONE << (MANTISSA_SIZE - i);
    APPLY_MASK(*tmp, mask);
    fprintf(file_ref, "%d %a\n", i, NEAREST_REAL(cpa));
  }
}

int main(void) {
  REAL a = 0.0;
  INT_T *tmp = (INT_T*)&a;
  *tmp = REAL_TEST_VALUE;
  generate_real_masked(&a);
  return 0;
}
