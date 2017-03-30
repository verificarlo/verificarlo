#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdint.h>

#include "libbitmask.h"
#include "../vfcwrapper/vfcwrapper.h"
#include "../common/mca_const.h"

static int MCALIB_T = 53;

//possible op values
#define MCA_ADD 1
#define MCA_SUB 2
#define MCA_MUL 3
#define MCA_DIV 4

// perform_bin_op: applies the binary operator (op) to (a) and (b)
// and stores the result in (res)
#define perform_bin_op(op, res, a, b)                               \
  switch (op){							    \
    case MCA_ADD: res=(a)+(b); break;                               \
    case MCA_MUL: res=(a)*(b); break;                               \
    case MCA_SUB: res=(a)-(b); break;                               \
    case MCA_DIV: res=(a)/(b); break;                               \
    default: perror("invalid operator in bitmask.\n"); abort();     \
  };

static float _mca_sbin(float a, float b, const int op) {
  uint32_t mask = FLOAT_MASK_1 & ~((1 << (FLOAT_PREC - MCALIB_T)) - 1);
  float res = 0.0;
  uint32_t *tmp = (uint32_t*)&res;

  printf("MCA_DBIN : MCALIB_T = %d, mask = %x\n", MCALIB_T, mask); 
  printf("a = %a, b = %a\n", a, b);

  perform_bin_op(op, res, a, b);

  printf("res = %f, tmp = %x\n", res, *tmp);

  *tmp = (*tmp) & mask;

  printf("res = %f, res = %a, tmp = %x\n", res, res, *tmp);

  return NEAREST_FLOAT(res);
}

static double _mca_dbin(double a, double b, const int op) {
  uint64_t mask = DOUBLE_MASK_1 & ~((1 << (DOUBLE_PREC - MCALIB_T)) - 1);
  double res = 0.0;
  uint64_t *tmp = (uint64_t*)&res;

  printf("MCA_DBIN : MCALIB_T = %d, mask = %lx\n", MCALIB_T, mask); 
  printf("a = %a, b = %a\n", a, b);

  perform_bin_op(op, res, a, b);

  printf("res = %f, tmp = %lx\n", res, *tmp);

  *tmp = (*tmp) & mask;

  printf("res = %f, res = %a, tmp = %lx\n", res, res, *tmp);
  
  return NEAREST_DOUBLE(res);
}

int main(int argc, char* argv[]){
  float a = 0.0;
  float b = 0.1;
  float res = 0.0;

  MCALIB_T = 1;
  res = _mca_sbin(a, b, MCA_ADD);
  
  printf("res = %f, res = %a\n", res ,res);  
  return 0;
}
