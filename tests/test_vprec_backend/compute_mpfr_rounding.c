/*
 * MPFR reference value generator for VPREC testing.
 *
 * Generates reference files for all (type, mode, range, precision) combinations.
 * Each reference line has the format:
 *   = op arg1 arg2 [arg3] => result
 *   ~ op arg1 arg2 [arg3] => result
 * where '=' means exact rounding (strict equality expected)
 * and '~' means a midpoint/tie-break case (1 ULP tolerance accepted).
 *
 * Backported from verrou/unitTest/checkVPREC/compute_mpfr_rounding.c
 *
 * Usage: compute_mpfr_rounding outputDirectory
 */

#include <errno.h>
#include <math.h>
#include <mpfr.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "fpPropImpl.c"

typedef enum { ieee, pb, ob, full, unknown_mode } vprec_mode_t;
typedef enum { float_type, double_type, unknown_type } fptype_t;

static bool verbose_mode = false;

void fprint_normalized_hex99_raw(mpfr_t x, fptype_t type, FILE *stream,
                                 const char *nl) {
  if (type == float_type) {
    float f = mpfr_get_flt(x, MPFR_RNDN);
    fprintf(stream, "%+.6a%s", f, nl);
  } else if (type == double_type) {
    double d = mpfr_get_d(x, MPFR_RNDN);
    fprintf(stream, "%+.13a%s", d, nl);
  }
}

void strLineToDoubleTab(double *tab, int nbArgs, char *string) {
  errno = 0;
  char *endptr = string;
  for (int i = 0; i < nbArgs; i++) {
    tab[i] = strtod(endptr, &endptr);
    if (errno != 0) {
      fprintf(stderr, "wrong float: %s", strerror(errno));
      exit(1);
    }
  }
}

mpfr_exp_t get_emax_for_mpfr(int range) {
  int e = 1 << (range - 1);
  return e;
}

mpfr_exp_t get_emin_for_mpfr(int range, int precision) {
  int e = 1 << (range - 1);
  /* emin formula from: stackoverflow.com/questions/38664778 */
  e = -e - precision + 3;
  return e;
}

void get_smallest_positive_subnormal_number(mpfr_t smallest_subnormal,
                                            int range, int precision) {
  mpfr_exp_t _emin = mpfr_get_emin();
  mpfr_exp_t _emax = mpfr_get_emax();

  int emx = (1 << (range - 1)) - 1;
  int emn = 1 - emx;

  mpfr_set_emin(mpfr_get_emin_min());
  mpfr_set_emax(mpfr_get_emax_max());

  mpfr_init2(smallest_subnormal, 256);
  mpfr_set_ui_2exp(smallest_subnormal, 1, emn - precision, MPFR_RNDN);

  mpfr_set_emin(_emin);
  mpfr_set_emax(_emax);

  double fpPropMinDenorm = floatMinDeNorm(range, precision);
  double mpfrMinDenorm = mpfr_get_d(smallest_subnormal, MPFR_RNDN);
  assert(fpPropMinDenorm == mpfrMinDenorm);
}

void get_smallest_normal_number(mpfr_t smallest_normal, int range) {
  mpfr_exp_t _emin = mpfr_get_emin();
  mpfr_exp_t _emax = mpfr_get_emax();

  int emx = (1 << (range - 1)) - 1;
  int emn = 1 - emx;

  mpfr_set_emin(mpfr_get_emin_min());
  mpfr_set_emax(mpfr_get_emax_max());

  mpfr_init2(smallest_normal, 256);
  mpfr_set_ui_2exp(smallest_normal, 1, emn, MPFR_RNDN);

  mpfr_set_emin(_emin);
  mpfr_set_emax(_emax);

  double fpPropMin = floatMinNorm(range, 0);
  double mpfrMin = mpfr_get_d(smallest_normal, MPFR_RNDN);
  assert(fpPropMin == mpfrMin);
}

void get_largest_positive_normal_number(mpfr_t largest_normal, int range,
                                        int precision) {
  mpfr_exp_t _emin = mpfr_get_emin();
  mpfr_exp_t _emax = mpfr_get_emax();

  int emx = (1 << (range - 1)) - 1;

  mpfr_set_emin(mpfr_get_emin_min());
  mpfr_set_emax(mpfr_get_emax_max());

  mpfr_init2(largest_normal, 256);

  mpfr_set_ui_2exp(largest_normal, 1, -precision, MPFR_RNDN);
  mpfr_ui_sub(largest_normal, 2, largest_normal, MPFR_RNDN);
  mpfr_mul_2ui(largest_normal, largest_normal, emx, MPFR_RNDN);

  mpfr_set_emin(_emin);
  mpfr_set_emax(_emax);

  double fpPropMax = floatMax(range, precision);
  double mpfrMax = mpfr_get_d(largest_normal, MPFR_RNDN);
  assert(fpPropMax == mpfrMax);
}

int nb_args_from_op(const char op) {
  switch (op) {
  case '+':
  case '-':
  case 'x':
  case '/':
    return 2;
  case 'f':
    return 3;
  default:
    fprintf(stderr, "Bad op %c\n", op);
    exit(1);
  }
}

void overUnderFlow(mpfr_t x, int range, int precision) {
  mpfr_t smallest_subnormal, smallest_normal, largest_normal;
  get_smallest_positive_subnormal_number(smallest_subnormal, range, precision);
  get_largest_positive_normal_number(largest_normal, range, precision);
  get_smallest_normal_number(smallest_normal, range);

  if (mpfr_cmpabs(x, largest_normal) > 0) {
    if (verbose_mode)
      fprintf(stderr, "Overflow detected\n");
    mpfr_set_inf(x, mpfr_sgn(x));
  } else {
    if (mpfr_cmpabs(x, smallest_normal) < 0) {
      if (verbose_mode)
        fprintf(stderr, "Subnormal detected limit(%e)\n",
                mpfr_get_d(smallest_normal, MPFR_RNDN));

      if (mpfr_cmpabs(x, smallest_subnormal) < 0) {
        if (verbose_mode)
          fprintf(stderr, "Underflow detected %e\n",
                  mpfr_get_d(smallest_subnormal, MPFR_RNDN));
        mpfr_set_zero(x, mpfr_sgn(x));
      }
    }
  }

  mpfr_clear(smallest_subnormal);
  mpfr_clear(smallest_normal);
  mpfr_clear(largest_normal);
}

void apply_operation(mpfr_t res, mpfr_t *args, const char op, fptype_t type) {
  mpfr_clear_flags();

  const int precision = (type == float_type) ? 23 : 52;
  const int range = (type == float_type) ? 8 : 11;
  mpfr_set_default_prec(precision + 1);

  mpfr_exp_t emx = get_emax_for_mpfr(range);
  mpfr_exp_t emn = get_emin_for_mpfr(range, precision);

  mpfr_t resLocal;
  mpfr_set_emax(emx);
  mpfr_set_emin(emn);
  mpfr_inits2(precision + 1, resLocal, (mpfr_ptr)0);

  int i = 0;
  switch (op) {
  case '+':
    i = mpfr_add(resLocal, args[0], args[1], MPFR_RNDN);
    break;
  case '-':
    i = mpfr_sub(resLocal, args[0], args[1], MPFR_RNDN);
    break;
  case 'x':
    i = mpfr_mul(resLocal, args[0], args[1], MPFR_RNDN);
    break;
  case '/':
    i = mpfr_div(resLocal, args[0], args[1], MPFR_RNDN);
    break;
  case 'f':
    i = mpfr_fma(resLocal, args[0], args[1], args[2], MPFR_RNDN);
    break;
  case '=':
    i = mpfr_set(resLocal, args[0], MPFR_RNDN);
    break;
  default:
    fprintf(stderr, "Bad op %c\n", op);
    exit(1);
  }
  i = mpfr_check_range(resLocal, i, MPFR_RNDN);
  mpfr_subnormalize(resLocal, i, MPFR_RNDN);

  overUnderFlow(resLocal, range, precision);

  mpfr_set(res, resLocal, MPFR_RNDN);
  mpfr_clear(resLocal);
}

bool vprec_rounding(mpfr_t x, int range, int precision) {
  double xDouble = mpfr_get_d(x, MPFR_RNDN);
  mpfr_clear_flags();
  mpfr_exp_t emx = get_emax_for_mpfr(range);
  mpfr_exp_t emn = get_emin_for_mpfr(range, precision);

  mpfr_t xRound;
  mpfr_set_emax(emx);
  mpfr_set_emin(emn);
  mpfr_inits2(precision + 1, xRound, (mpfr_ptr)0);
  int i = mpfr_set(xRound, x, MPFR_RNDN);
  i = mpfr_check_range(xRound, i, MPFR_RNDN);
  i = mpfr_subnormalize(xRound, i, MPFR_RNDN);

  overUnderFlow(xRound, range, precision);

  double xRoundDouble = mpfr_get_d(xRound, MPFR_RNDN);
  double xDiff = fabs(xDouble - xRoundDouble);
  bool midPoint = false;
  if (2. * xDiff == getUlp(xRoundDouble, range, precision)) {
    if (fabs(xRoundDouble) != INFINITY) {
      printf("Warning mid-point rounding: %.17e %.a\n", xDouble, xDouble);
    }
    midPoint = true;
  }

  mpfr_set(x, xRound, MPFR_RNDN);
  mpfr_clear(xRound);
  return midPoint;
}

void generate_ref(double *arg, char op, fptype_t type, FILE *fp,
                  vprec_mode_t mode, int range, int precision) {
  const int working_precision = (type == float_type) ? 24 : 53;

  int nbArg = nb_args_from_op(op);
  mpfr_t marg[3];
  mpfr_t mres;
  mpfr_t marg_org[3];
  bool midPoint = false;
  mpfr_inits2(working_precision, mres, (mpfr_ptr)0);
  for (int i = 0; i < nbArg; i++) {
    mpfr_inits2(working_precision, marg[i], (mpfr_ptr)0);
    if (type == float_type) {
      mpfr_set_flt(marg[i], arg[i], MPFR_RNDN);
    } else if (type == double_type) {
      mpfr_set_d(marg[i], arg[i], MPFR_RNDN);
    }
    apply_operation(marg[i], &(marg[i]), '=', type);
    mpfr_inits2(working_precision, marg_org[i], (mpfr_ptr)0);
    mpfr_set(marg_org[i], marg[i], MPFR_RNDN);
  }

  if (mode == pb || mode == full) {
    for (int i = 0; i < nbArg; i++) {
      midPoint = midPoint | vprec_rounding(marg[i], range, precision);
    }
  }

  apply_operation(mres, marg, op, type);

  if (mode == ob || mode == full) {
    midPoint = midPoint | vprec_rounding(mres, range, precision);
  }

  if (midPoint) {
    fprintf(fp, "~ ");
  } else {
    fprintf(fp, "= ");
  }

  fprintf(fp, "%c ", op);
  for (int i = 0; i < nbArg - 1; i++) {
    fprint_normalized_hex99_raw(marg_org[i], type, fp, " ");
  }
  fprint_normalized_hex99_raw(marg_org[nbArg - 1], type, fp, " => ");
  fprint_normalized_hex99_raw(mres, type, fp, "\n");

  mpfr_clear(mres);
  for (int i = 0; i < nbArg; i++) {
    mpfr_clear(marg[i]);
    mpfr_clear(marg_org[i]);
  }
}

int main(int argc, char *argv[]) {

  if (argc != 2) {
    fprintf(stderr, "usage: compute_mpfr_rounding outputDirectory\n");
    exit(1);
  }

  char *outputRep = argv[1];

  fptype_t fpTypeTab[] = {float_type, double_type};
  char *fpTypeTabStr[] = {"float", "double"};

  int fpRangeMin[] = {2, 2};
  int fpRangeMax[] = {8, 11};

  int fpPrecisionMin[] = {2, 2};
  int fpPrecisionMax[] = {23, 52};

  /* Operations: +, -, x, /, f (fma). No sqrt. */
  char opTabStr[] = {'+', '-', 'x', '/', 'f'};

  vprec_mode_t modeTab[] = {pb, ob, full};
  char *modeTabStr[] = {"ib", "ob", "full"};

  for (int indexMode = 0; indexMode < 3; indexMode++) {
    vprec_mode_t vprec_mode = modeTab[indexMode];
    printf("Generate %s reference\n", modeTabStr[indexMode]);

    for (int indexType = 0; indexType < 2; indexType++) {
      fptype_t fpType = fpTypeTab[indexType];
      printf("Generate %s reference\n", fpTypeTabStr[indexType]);

      for (int vprec_range = fpRangeMin[indexType];
           vprec_range <= fpRangeMax[indexType]; vprec_range++) {
        for (int vprec_precision = fpPrecisionMin[indexType];
             vprec_precision <= fpPrecisionMax[indexType]; vprec_precision++) {
          printf("\treference for E%dM%d\n", vprec_range, vprec_precision);

          char line[512];
          char inputPointFile[512];
          snprintf(inputPointFile, 512, "input_%i.txt", vprec_range);

          FILE *listOfPoints = fopen(inputPointFile, "r");
          if (listOfPoints == NULL) {
            printf("Error while opening input point file: %s\n",
                   inputPointFile);
            return EXIT_FAILURE;
          }

          char outRefName[512];
          snprintf(outRefName, 512, "%s/mpfr_%s_%s_E%dM%d", outputRep,
                   fpTypeTabStr[indexType], modeTabStr[indexMode], vprec_range,
                   vprec_precision);
          printf("path:  %s\n", outRefName);
          FILE *refFile = fopen(outRefName, "w");

          while (fgets(line, 512, listOfPoints)) {
            double args[3];
            strLineToDoubleTab(args, 3, line);

            for (int indexOp = 0; indexOp < 5; indexOp++) {
              generate_ref(args, opTabStr[indexOp], fpType, refFile, vprec_mode,
                           vprec_range, vprec_precision);
            }
          }
          fclose(listOfPoints);
          fclose(refFile);
        }
      }
    }
  }
}
