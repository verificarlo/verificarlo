/*
 * VPREC rounding computation.
 * Compiled with verificarlo-c so all FP operations are instrumented by VPREC.
 *
 * Reads a reference file (to extract operations and input arguments),
 * performs each operation, and outputs the VPREC result to stdout.
 *
 * REAL is defined via -DREAL=float or -DREAL=double at compile time.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

int nb_args(char op) {
  switch (op) {
  case '+':
  case '-':
  case 'x':
  case '/':
    return 2;
  case 'f':
    return 3;
  default:
    return -1;
  }
}

REAL perform_op(char op, REAL *args) {
  switch (op) {
  case '+':
    return args[0] + args[1];
  case '-':
    return args[0] - args[1];
  case 'x':
    return args[0] * args[1];
  case '/':
    return args[0] / args[1];
  case 'f':
    /* fma(a, b, c) = a*b + c with a single rounding */
    if (sizeof(REAL) == sizeof(float))
      return fmaf(args[0], args[1], args[2]);
    else
      return fma(args[0], args[1], args[2]);
  default:
    fprintf(stderr, "Unsupported op: %c\n", op);
    exit(EXIT_FAILURE);
  }
}

int main(int argc, char *argv[]) {

  if (argc < 2) {
    fprintf(stderr, "usage: compute_vprec_rounding reference_file [max_samples]\n");
    exit(EXIT_FAILURE);
  }

  FILE *ref = fopen(argv[1], "r");
  if (!ref) {
    perror(argv[1]);
    exit(EXIT_FAILURE);
  }

  int max_samples = 0; /* 0 means unlimited */
  if (argc >= 3)
    max_samples = atoi(argv[2]);

  char line[512];
  int count = 0;
  while (fgets(line, sizeof(line), ref)) {
    if (max_samples > 0 && count >= max_samples)
      break;

    /* Reference line format:
     * = + +0x1.3be88fp-2 -0x1.2b46c1p-3 => +0x1.4c8a5dp-3
     * ~ f +0x1.abcp+1 -0x1.123p+2 +0x1.789p+3 => +0x1.fedp+4
     *
     * line[0] = '=' or '~' (midpoint marker)
     * line[1] = ' '
     * line[2] = operation character
     * line[3] = ' '
     * line[4..] = arguments separated by spaces, then " => " and reference result
     */
    char op = line[2];
    int nargs = nb_args(op);

    if (nargs < 0) {
      /* Unsupported op (e.g., 's' for sqrt), output SKIP marker */
      printf("SKIP\n");
      count++;
      continue;
    }

    /* Parse arguments */
    char *ptr = line + 4;
    char *endptr;
    REAL args[3];
    for (int i = 0; i < nargs; i++) {
      if (sizeof(REAL) == sizeof(float))
        args[i] = strtof(ptr, &endptr);
      else
        args[i] = strtod(ptr, &endptr);
      ptr = endptr;
    }

    REAL result = perform_op(op, args);

    /* Output result in hex format matching reference precision */
    if (sizeof(REAL) == sizeof(float))
      printf("%+.6a\n", (double)result);
    else
      printf("%+.13a\n", result);

    count++;
  }

  fclose(ref);
  return EXIT_SUCCESS;
}
