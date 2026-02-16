/*
 * VPREC result checker.
 *
 * Reads a pre-generated MPFR reference file and VPREC output file,
 * compares results using ULP-based tolerance with midpoint awareness.
 *
 * Usage: check_output reference_file vprec_output_file range precision [max_samples]
 *
 * Mode (ob/ib/full) and type (float/double) are detected from the reference filename.
 * Comparison rules:
 *   '=' lines: strict equality expected
 *   '~' lines (midpoint): up to 1 ULP difference accepted
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "fpPropImpl.c"

typedef enum { equal_exact = 0, equal_ulp = 1, inf_ulp = 2, inf_2ulp = 3, sup_2ulp = 4 } cmp_res_t;

cmp_res_t cmpResult(double ref, double a, int range, int precision) {
  if (ref == a)
    return equal_exact;
  if (isnan(ref) && isnan(a))
    return equal_exact;
  if (ref == 0) {
    fprintf(stderr, "  0 expected but got %.17e (%a)\n", a, a);
    return sup_2ulp;
  }

  double absError = fabs(ref - a);
  double ulp = getUlp(ref, range, precision);

  if (absError == ulp)
    return equal_ulp;
  else if (absError < ulp)
    return inf_ulp;
  else if (absError <= 2.0 * ulp)
    return inf_2ulp;
  else
    return sup_2ulp;
}

void printError(double ref, double a, int range, int precision) {
  double absError = fabs(ref - a);
  double relError = (ref != 0) ? fabs((ref - a) / ref) : absError;
  double ulp = getUlp(ref, range, precision);
  fprintf(stderr, "  ref   : %+.13a  %.17e\n", ref, ref);
  fprintf(stderr, "  vprec : %+.13a  %.17e\n", a, a);
  fprintf(stderr, "  abs   : %+.13a  %.2f ulp\n", absError, absError / ulp);
  fprintf(stderr, "  rel   : %.17e\n", relError);
}

int main(int argc, char *argv[]) {
  if (argc < 5) {
    fprintf(stderr,
            "usage: check_output reference_file vprec_output range precision [max_samples]\n");
    exit(EXIT_FAILURE);
  }

  char *ref_filename = argv[1];
  char *vprec_filename = argv[2];
  int range = atoi(argv[3]);
  int precision = atoi(argv[4]);
  int max_samples = 0;
  if (argc >= 6)
    max_samples = atoi(argv[5]);

  /* Detect mode from reference filename */
  bool isOB = (strstr(ref_filename, "_ob_") != NULL);
  bool isIB = (strstr(ref_filename, "_ib_") != NULL);
  bool isFULL = (strstr(ref_filename, "_full_") != NULL);
  if (!(isOB || isIB || isFULL)) {
    fprintf(stderr, "Cannot detect mode (ob/ib/full) from filename: %s\n", ref_filename);
    exit(EXIT_FAILURE);
  }

  FILE *ref_fp = fopen(ref_filename, "r");
  FILE *vprec_fp = fopen(vprec_filename, "r");
  if (!ref_fp) {
    perror(ref_filename);
    exit(EXIT_FAILURE);
  }
  if (!vprec_fp) {
    perror(vprec_filename);
    exit(EXIT_FAILURE);
  }

  char ref_line[512], vprec_line[512];
  int counterOK = 0, counterOKTol = 0, counterKO = 0, counterSkip = 0;
  int count = 0;

  while (fgets(ref_line, sizeof(ref_line), ref_fp) &&
         fgets(vprec_line, sizeof(vprec_line), vprec_fp)) {
    if (max_samples > 0 && count >= max_samples)
      break;

    /* Skip lines for unsupported ops (sqrt) */
    if (strncmp(vprec_line, "SKIP", 4) == 0) {
      counterSkip++;
      count++;
      continue;
    }

    /* Parse reference line: marker op args => expected */
    char marker = ref_line[0]; /* '=' or '~' */
    bool exact = (marker == '=');
    char op = ref_line[2];

    /* Find the expected result after " => " separator */
    char *arrow = strstr(ref_line, "=> ");
    if (!arrow) {
      fprintf(stderr, "Bad reference format at line %d: %s", count + 1, ref_line);
      exit(EXIT_FAILURE);
    }
    double ref_val = strtod(arrow + 3, NULL);

    /* Parse vprec result */
    double vprec_val = strtod(vprec_line, NULL);

    cmp_res_t cmpRes = cmpResult(ref_val, vprec_val, range, precision);

    if (exact) {
      if (cmpRes == equal_exact) {
        counterOK++;
      } else {
        counterKO++;
        fprintf(stderr, "FAIL (exact) line %d op=%c:\n", count + 1, op);
        printError(ref_val, vprec_val, range, precision);
        fprintf(stderr, "  refLine: %s", ref_line);
      }
    } else {
      /* Midpoint case ('~') */
      switch (cmpRes) {
      case equal_exact:
        counterOK++;
        break;
      case equal_ulp:
        counterOKTol++;
        break;
      case inf_ulp:
        if (isOB) {
          counterKO++;
          fprintf(stderr, "FAIL (midpoint/ob) line %d op=%c:\n", count + 1, op);
          printError(ref_val, vprec_val, range, precision);
        } else {
          /* IB or FULL: sub-ULP difference accepted for midpoints */
          counterOKTol++;
        }
        break;
      case inf_2ulp:
      case sup_2ulp:
        counterKO++;
        fprintf(stderr, "FAIL (midpoint/>ulp) line %d op=%c:\n", count + 1, op);
        printError(ref_val, vprec_val, range, precision);
        fprintf(stderr, "  refLine: %s", ref_line);
        break;
      default:
        fprintf(stderr, "error in switch\n");
        break;
      }
    }
    count++;
  }

  printf("E%dM%d: OK=%d  OK(tol)=%d  KO=%d  skip=%d  total=%d\n",
         range, precision, counterOK, counterOKTol, counterKO, counterSkip, count);

  fclose(ref_fp);
  fclose(vprec_fp);

  if (counterKO != 0)
    return EXIT_FAILURE;
  return EXIT_SUCCESS;
}
