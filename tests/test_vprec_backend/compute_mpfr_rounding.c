#include <errno.h>
#include <mpfr.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

static const char verbose_env[] = "VERBOSE_MODE";
static const char vprec_range_env[] = "VERIFICARLO_VPREC_RANGE";
static const char precision_env[] = "VERIFICARLO_PRECISION";
static const char vprec_mode_env[] = "VERIFICARLO_VPREC_MODE";

typedef enum { ieee, pb, ob, full } vprec_mode;

static bool verbose_mode = false;

bool get_verbose() {
  char *env = getenv(verbose_env);
  return (env != NULL);
}

vprec_mode get_mode() {
  char *env = getenv(vprec_mode_env);
  if (env == NULL) {
    fprintf(stderr, "%s is empty\n", vprec_mode_env);
    exit(1);
  }
  if (strcasecmp(env, "ieee") == 0) {
    return ieee;
  } else if (strcasecmp(env, "ib") == 0) {
    return pb;
  } else if (strcasecmp(env, "ob") == 0) {
    return ob;
  } else if (strcasecmp(env, "full") == 0) {
    return full;
  } else {
    fprintf(stderr, "Bad vprec mode: %s\n", env);
    exit(1);
  }
}

long get_range() {
  char *env = getenv(vprec_range_env);
  if (env == NULL) {
    fprintf(stderr, "%s is empty\n", vprec_range_env);
    exit(1);
  }

  errno = 0;
  char *endptr;
  long range = strtol(env, &endptr, 10);
  if (errno != 0) {
    fprintf(stderr, "wrong %s: %s", vprec_range_env, strerror(errno));
    exit(1);
  }

  return range;
}

long get_precision() {
  char *env = getenv(precision_env);
  if (env == NULL) {
    fprintf(stderr, "%s is empty\n", precision_env);
    exit(1);
  }

  errno = 0;
  char *endptr;
  long precision = strtol(env, &endptr, 10);
  if (errno != 0) {
    fprintf(stderr, "wrong %s: %s", precision_env, strerror(errno));
    exit(1);
  }

  return precision;
}

double get_float(const char *string) {
  errno = 0;
  char *endptr;
  double x = strtod(string, &endptr);
  if (errno != 0) {
    fprintf(stderr, "wrong float: %s", strerror(errno));
    exit(1);
  }
  return x;
}

long get_int(const char *string) {
  errno = 0;
  char *endptr;
  long x = strtol(string, &endptr, 10);
  if (errno != 0) {
    fprintf(stderr, "wrong int: %s", strerror(errno));
    exit(1);
  }
  return x;
}

mpfr_exp_t get_emax() {
  int range = get_range();
  int emax = 1 << (range - 1);
  return emax;
}

mpfr_exp_t get_emin() {
  int range = get_range();
  int precision = get_precision();
  int emin = 1 << (range - 1);
  emin = -emin - precision + 4;
  return emin;
}

void apply_operation(mpfr_t res, mpfr_t a, mpfr_t b, const char op) {
  int i = 0;
  switch (op) {
  case '+':
    i = mpfr_add(res, a, b, MPFR_RNDN);
    break;
  case '-':
    i = mpfr_sub(res, a, b, MPFR_RNDN);
    break;
  case 'x':
    i = mpfr_mul(res, a, b, MPFR_RNDN);
    break;
  case '/':
    i = mpfr_div(res, a, b, MPFR_RNDN);
    break;
  default:
    fprintf(stderr, "Bad op %c\n", op);
    exit(1);
  }
  mpfr_subnormalize(res, i, MPFR_RNDN);
}

void intermediate_rounding(mpfr_t x, mpfr_prec_t precision) {
  mpfr_clear_flags();
  int i = mpfr_prec_round(x, precision, MPFR_RNDN);
  mpfr_check_range(x, i, MPFR_RNDN);
  if (mpfr_overflow_p()) {
    if (verbose_mode)
      fprintf(stderr, "Overflow detected\n");
    mpfr_set_inf(x, mpfr_sgn(x));
  } else if (mpfr_underflow_p()) {
    if (verbose_mode)
      fprintf(stderr, "Underflow detected\n");
    mpfr_set_zero(x, mpfr_sgn(x));
  }
}

void compute_float(float a, float b, char op) {

  vprec_mode mode = get_mode();
  mpfr_prec_t precision = get_precision();
  mpfr_exp_t emax = get_emax();
  mpfr_exp_t emin = get_emin();

  mpfr_t ma, mb, mres;
  mpfr_inits2(24, mres, ma, mb, (mpfr_ptr)0);
  mpfr_set_flt(ma, a, MPFR_RNDN);
  mpfr_set_flt(mb, b, MPFR_RNDN);

  if (verbose_mode) {
    mpfr_printf("[MPFR] (before rounding) a=%Ra\n", ma);
    mpfr_printf("[MPFR] (before rounding) b=%Ra\n", mb);
  }

  if (mode == pb || mode == full) {
    mpfr_clear_flags();
    mpfr_set_emax(emax);
    mpfr_set_emin(emin);
    intermediate_rounding(ma, precision);
    intermediate_rounding(mb, precision);
  }

  if (verbose_mode) {
    mpfr_printf("[MPFR] (after rounding) a=%Ra\n", ma);
    mpfr_printf("[MPFR] (after rounding) b=%Ra\n", mb);
  }

  mpfr_clear_flags();
  mpfr_set_emax(mpfr_get_emax_max());
  mpfr_set_emin(mpfr_get_emin_min());

  apply_operation(mres, ma, mb, op);

  if (mode == ob || mode == full) {
    mpfr_clear_flags();
    mpfr_set_emax(emax);
    mpfr_set_emin(emin);
    intermediate_rounding(mres, precision);
  }

  mpfr_printf("%Ra\n", mres);

  //   mpfr_clear(ma);
  //   mpfr_clear(mb);
  //   mpfr_clear(mres);
}

void compute_double(double a, double b, char op) {

  vprec_mode mode = get_mode();
  mpfr_prec_t precision = get_precision();
  mpfr_exp_t emax = get_emax();
  mpfr_exp_t emin = get_emin();

  mpfr_t ma, mb, mres;
  mpfr_inits2(53, mres, ma, mb, (mpfr_ptr)0);
  mpfr_set_d(ma, a, MPFR_RNDN);
  mpfr_set_d(mb, b, MPFR_RNDN);

  if (verbose_mode) {
    mpfr_printf("[MPFR] (before rounding) a=%Ra\n", ma);
    mpfr_printf("[MPFR] (before rounding) b=%Ra\n", mb);
  }

  if (mode == pb || mode == full) {
    mpfr_clear_flags();
    mpfr_set_emax(emax);
    mpfr_set_emin(emin);
    intermediate_rounding(ma, precision);
    intermediate_rounding(mb, precision);
  }

  if (verbose_mode) {
    mpfr_printf("[MPFR] (after rounding) a=%Ra\n", ma);
    mpfr_printf("[MPFR] (after rounding) b=%Ra\n", mb);
  }

  mpfr_clear_flags();
  mpfr_set_emax(mpfr_get_emax_max());
  mpfr_set_emin(mpfr_get_emin_min());

  apply_operation(mres, ma, mb, op);

  if (mode == ob || mode == full) {
    mpfr_clear_flags();
    mpfr_set_emax(emax);
    mpfr_set_emin(emin);
    intermediate_rounding(mres, precision);
  }

  mpfr_printf("%Ra\n", mres);

  //   mpfr_clear(ma);
  //   mpfr_clear(mb);
  //   mpfr_clear(mres);
}

int main(int argc, char *argv[]) {

  if (argc != 5) {
    fprintf(stderr, "usage: compute_mpfr_rounding a b op type\n");
    exit(1);
  }

  double a = get_float(argv[1]);
  double b = get_float(argv[2]);
  char op = argv[3][0];
  const char *type = argv[4];

  verbose_mode = get_verbose();

  if (verbose_mode) {
    fprintf(stderr, "[Input] a=%a\n", a);
    fprintf(stderr, "[Input] b=%a\n", b);
  }

  if (strcasecmp(type, "float") == 0) {
    compute_float(a, b, op);
  } else if (strcasecmp(type, "double") == 0) {
    compute_double(a, b, op);
  } else {
    fprintf(stderr, "Bad type: %s\n", type);
    exit(1);
  }
}