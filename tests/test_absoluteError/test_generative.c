#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <time.h>

#include "../../src/common/float_const.h"
#include "../../src/common/float_struct.h"
#include "../../src/common/float_utils.h"

#define DEBUG_MODE 0

double applyOp_double(char op, double a, double b) {
  double res = 0.0;

  switch (op) {
  case '+':
    res = a + b;
    break;
  case '-':
    res = a - b;
    break;
  case '*':
    res = a * b;
    break;
  case '/':
    res = a / b;
    break;
  default:
    fprintf(stderr, "Error: unknown operation\n");
    abort();
    break;
  }

  return res;
}

float applyOp_float(char op, float a, float b) {
  float res = 0.0;

  switch (op) {
  case '+':
    res = a + b;
    break;
  case '-':
    res = a - b;
    break;
  case '*':
    res = a * b;
    break;
  case '/':
    res = a / b;
    break;
  default:
    fprintf(stderr, "Error: unknown operation\n");
    abort();
    break;
  }

  return res;
}

int main(int argc, char const *argv[]) {
  if (argc < 5) {
    fprintf(stderr, "./test_generative operation type number_of_tests "
                    "absolute_error_exponent\n");
    abort();
  }

  int ret = 0;
  size_t i;

  char op = argv[1][0];
  int type = atoi(argv[2]);
  int nbTests = atoi(argv[3]);
  int absErr_exp = atoi(argv[4]);

  // srand(time(0));
  srand(0);

  for (i = 0; i < nbTests; i++) {
    // generate two random numbers
    //  create the floating point numbers
    binary32 a_b32 = {.f32 = 1.0};
    binary32 b_b32 = {.f32 = 1.0};
    binary64 a_b64 = {.f64 = 1.0};
    binary64 b_b64 = {.f64 = 1.0};

    //  create random mantissas for the two numbers
    unsigned long int tmp, tmp2;

    //  create a
    if (type == 0) {
      // float
      tmp = rand() & FLOAT_GET_PMAN;
      a_b32.u32 += (unsigned int)tmp;
    } else {
      // double
      tmp = rand() & FLOAT_GET_PMAN;
      tmp2 = rand() & ((1 << (DOUBLE_PMAN_SIZE - FLOAT_PMAN_SIZE)) - 1);
      a_b64.u64 += (unsigned long long int)tmp;
      a_b64.u64 += (unsigned long long int)(tmp2 << (FLOAT_PMAN_SIZE));
    }

    //  create b
    if (type == 0) {
      // float
      tmp = rand() & FLOAT_GET_PMAN;
      b_b32.u32 += (unsigned int)tmp;
    } else {
      // double
      tmp = rand() & FLOAT_GET_PMAN;
      tmp2 = rand() & ((1 << (DOUBLE_PMAN_SIZE - FLOAT_PMAN_SIZE)) - 1);
      b_b64.u64 += (unsigned long long int)tmp;
      b_b64.u64 += (unsigned long long int)(tmp2 << (FLOAT_PMAN_SIZE));
    }

    // apply the operation
    binary32 res_b32;
    binary64 res_b64;

    if (type == 0) {
      res_b32.f32 = applyOp_float(op, a_b32.f32, b_b32.f32);
    } else {
      res_b64.f64 = applyOp_double(op, a_b64.f64, b_b64.f64);
    }

    // check if the result is correct
    unsigned int absErr_mask_float;
    unsigned long long int absErr_mask_double;

    if (type == 0) {
      // adjust the mask, if the result changed exponent
      int mask_adjust = floor(log2f(res_b32.f32));
      int shiftAmount;

      if(abs(absErr_exp)+mask_adjust > FLOAT_PMAN_SIZE)
        shiftAmount = 0;
      else
        shiftAmount = FLOAT_PMAN_SIZE - abs(absErr_exp) - mask_adjust;
      absErr_mask_float = (1U << shiftAmount) - 1;

      binary32 res_b32_check = {.f32 = res_b32.f32};
      binary32 res_b32_ref = {.f32 = res_b32.f32};

      if (mask_adjust >= absErr_exp) {
        // should apply the mask to test for the correct result
        res_b32_check.u32 = res_b32_check.u32 & absErr_mask_float;
        if(shiftAmount > 0) {
          res_b32_ref.u32 = res_b32_ref.u32 & ~absErr_mask_float;
        }
      } else {
        // should create the correct result by hand
        if ((mask_adjust - absErr_exp) == -1) {
          res_b32_check.f32 = res_b32.f32 - exp2f(absErr_exp);
          res_b32_ref.f32 = exp2f(absErr_exp);
        } else {
          res_b32_check.f32 = 0;
          res_b32_ref.f32 = 0;
        }
      }

      if (res_b32_check.u32 != 0) {
#if DEBUG_MODE > 0
        printf("Fail!\n");
#endif
        // ret = 1;
        ret += 1;
      } else {
#if DEBUG_MODE > 0
        printf("Success!\n");
#endif
        // ret = 0;
      }
#if DEBUG_MODE > 0
      printf("\ta=%56.53f\n", a_b32.f32);
      printf("\tb=%56.53f\n", b_b32.f32);
      printf("\top=%c\n", op);
      printf("\ttype=%s\n", (type == 0) ? "float" : "double");
      printf("\terror threshold=2^%d\n", absErr_exp);
      printf("\tresult=%56.53f\n", res_b32.f32);
      printf("\texpected result=%56.53f\n", res_b32_ref.f32);
#endif
    } else {
      // adjust the mask, if the result changed exponent
      int mask_adjust = floor(log2(res_b64.f64));
      int shiftAmount;

      if(abs(absErr_exp)+mask_adjust > DOUBLE_PMAN_SIZE)
        shiftAmount = 0;
      else
        shiftAmount = DOUBLE_PMAN_SIZE - abs(absErr_exp) - mask_adjust;
      absErr_mask_double = (1ULL << shiftAmount) - 1;

      binary64 res_b64_check = {.f64 = res_b64.f64};
      binary64 res_b64_ref = {.f64 = res_b64.f64};

      if (mask_adjust >= absErr_exp) {
        // should apply the mask to test for the correct result
        res_b64_check.u64 = res_b64_check.u64 & absErr_mask_double;
        if(shiftAmount > 0) {
          res_b64_ref.u64 = res_b64_ref.u64 & ~absErr_mask_double;
        }
      } else {
        // should create the correct result by hand
        if ((mask_adjust - absErr_exp) == -1) {
          res_b64_check.f64 = res_b64.f64 - exp2(absErr_exp);
          res_b64_ref.f64 = exp2(absErr_exp);
        } else {
          res_b64_check.f64 = 0;
          res_b64_ref.f64 = 0;
        }
      }

      if (res_b64_check.u64 != 0) {
#if DEBUG_MODE > 0
        printf("Fail!\n");
#endif
        // ret = 1;
        ret += 1;
      } else {
#if DEBUG_MODE > 0
        printf("Success!\n");
#endif
        ret = 0;
      }
#if DEBUG_MODE > 0
      printf("\ta=%56.53lf\n", a_b64.f64);
      printf("\tb=%56.53f\n", b_b64.f64);
      printf("\top=%c\n", op);
      printf("\ttype=%s\n", (type == 0) ? "float" : "double");
      printf("\terror threshold=2^%d\n", absErr_exp);
      printf("\tresult=%56.53f\n", res_b64.f64);
      printf("\texpected result=%56.53f\n", res_b64_ref.f64);
#endif
    }
  }

  if (ret > 0)
    printf("Fail!\n");
  else
    printf("Success!\n");
  printf("\ttests failed=%d\n", ret);
  printf("\ttests total=%d\n", nbTests);

  return ret;
}
