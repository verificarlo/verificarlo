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

  srand(time(0));
  // srand(0);

  for (i = 0; i < nbTests; i++) {
    // generate two random numbers
    //  create the floating point numbers
    binary32 a_b32 = {.f32 = (float)rand() - (float)RAND_MAX / (float)2.0};
    binary32 b_b32 = {.f32 = (float)rand() - (float)RAND_MAX / (float)2.0};
    binary64 a_b64 = {.f64 = (double)rand() - (double)RAND_MAX / 2.0};
    binary64 b_b64 = {.f64 = (double)rand() - (double)RAND_MAX / 2.0};

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
      a_b64.u64 += (unsigned long int)tmp;
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
      b_b64.u64 += (unsigned long int)tmp;
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
    if (type == 0) {
      float absErr_max = exp2f(absErr_exp);
      float absErr_max_half = exp2f(absErr_exp - 1);
      float absErr_max_quarter = exp2f(absErr_exp - 2);
      binary32 res_b32_check = {.f32 = res_b32.f32};
      binary32 res_b32_check_v2 = {.f32 = res_b32.f32};

      // add 2^{absErr_exp-1} to the result
      //  this should give a result that is within 2^{absErr_exp} of the result
      //  depending on the rounding mode used by vprec
      res_b32_check.f32 = applyOp_float(op, res_b32_check.f32, absErr_max_half);
      if (fabsf(res_b32_check.f32 - res_b32.f32) > absErr_max) {
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

      // add 2^{absErr_exp-2} to the result
      //  this should give a result that to the original result
      res_b32_check_v2.f32 =
          applyOp_float(op, res_b32_check_v2.f32, absErr_max_quarter);
      if ((res_b32_check_v2.f32 - res_b32.f32) != 0) {
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
      printf("\tresult incremented by 1/2 of error threshold=%56.53f\n",
             res_b32_check.f32);
      printf("\tresult incremented by 1/4 of error threshold=%56.53f\n",
             res_b32_check_v2.f32);
#endif
    } else {
      double absErr_max = exp2(absErr_exp);
      double absErr_max_half = exp2(absErr_exp - 1);
      double absErr_max_quarter = exp2(absErr_exp - 2);
      binary64 res_b64_check = {.f64 = res_b64.f64};
      binary64 res_b64_check_v2 = {.f64 = res_b64.f64};

      // add 2^{absErr_exp-1} to the result
      //  this should give a result that is within 2^{absErr_exp} of the result
      //  depending on the rounding mode used by vprec
      res_b64_check.f64 =
          applyOp_double(op, res_b64_check.f64, absErr_max_half);
      if (fabs(res_b64_check.f64 - res_b64.f64) > absErr_max) {
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

      // add 2^{absErr_exp-2} to the result
      //  this should give a result that to the original result
      res_b64_check_v2.f64 =
          applyOp_double(op, res_b64_check_v2.f64, absErr_max_quarter);
      if ((res_b64_check_v2.f64 - res_b64.f64) != 0) {
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
      printf("\ta=%56.53lf\n", a_b64.f64);
      printf("\tb=%56.53lf\n", b_b64.f64);
      printf("\top=%c\n", op);
      printf("\ttype=%s\n", (type == 0) ? "float" : "double");
      printf("\terror threshold=2^%d\n", absErr_exp);
      printf("\tresult=%56.53lf\n", res_b64.f64);
      printf("\tresult incremented by 1/2 error threshold=%56.53lf\n",
             res_b64_check.f64);
      printf("\tresult incremented by 1/4 error threshold=%56.53lf\n",
             res_b64_check_v2.f64);
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
