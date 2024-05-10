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

typedef enum { ieee, pb, ob, full, unknown_mode } vprec_mode_t;
typedef enum { float_type, double_type, unknown_type } fptype_t;

static const char *mode_names[] = {[ieee] = "ieee",
                                   [pb] = "pb",
                                   [ob] = "ob",
                                   [full] = "full",
                                   [unknown_mode] = "unknown"};

static bool verbose_mode = false;
static int vprec_precision = -1;
static int vprec_range = -1;
static vprec_mode_t vprec_mode = unknown_mode;

mpfr_t largest_normal, smallest_subnormal;

#define HEADER_FMT "%-32s"
#define VAR_FMT "%22s"

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

void fprint_normalized_hex99(const char *msg, const char *name, mpfr_t x,
                             fptype_t type, FILE *stream) {
  fprintf(stream, HEADER_FMT " " VAR_FMT, msg, name);
  fprint_normalized_hex99_raw(x, type, stream, "\n");
}

void fprint_normalized_hex99_2(const char *msg, const char *name_x,
                               const char *name_y, mpfr_t x, mpfr_t y,
                               fptype_t type, FILE *stream) {
  fprintf(stream, HEADER_FMT " " VAR_FMT, msg, name_x);
  fprint_normalized_hex99_raw(x, type, stream, " ");
  fprintf(stream, VAR_FMT, name_y);
  fprint_normalized_hex99_raw(y, type, stream, "\n");
}

void print_normalized_hex99_debug(const char *msg, const char *name, mpfr_t x,
                                  fptype_t type) {
  fprint_normalized_hex99(msg, name, x, type, stderr);
}

void print_normalized_hex99(const char *msg, const char *name, mpfr_t x,
                            fptype_t type) {
  fprint_normalized_hex99(msg, name, x, type, stdout);
}

void print_normalized_hex99_debug_2(const char *msg, const char *name_x,
                                    const char *name_y, mpfr_t x, mpfr_t y,
                                    fptype_t type) {
  fprint_normalized_hex99_2(msg, name_x, name_y, x, y, type, stderr);
}

void print_debug(const char *msg, const char *name, const char *value) {
  fprintf(stderr, HEADER_FMT " " VAR_FMT "%-10s\n", msg, name, value);
}

void print_debug_float(const char *msg, const char *name, double x) {
  fprintf(stderr, HEADER_FMT " " VAR_FMT "%+.13a\n", msg, name, x);
}

void print_debug_int(const char *msg, const char *name, int x) {
  fprintf(stderr, HEADER_FMT " " VAR_FMT "%d\n", msg, name, x);
}

void print_sep() { fprintf(stderr, "------------------------------\n"); }

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

bool get_verbose() {
  char *env = getenv(verbose_env);
  return (env != NULL);
}

vprec_mode_t get_mode() {
  if (vprec_mode != unknown_mode) {
    return vprec_mode;
  }

  char *env = getenv(vprec_mode_env);
  vprec_mode_t mode = ieee;
  if (env == NULL) {
    fprintf(stderr, "%s is empty\n", vprec_mode_env);
    exit(1);
  }
  if (strcasecmp(env, "ieee") == 0) {
    mode = ieee;
  } else if (strcasecmp(env, "ib") == 0) {
    mode = pb;
  } else if (strcasecmp(env, "ob") == 0) {
    mode = ob;
  } else if (strcasecmp(env, "full") == 0) {
    mode = full;
  } else {
    fprintf(stderr, "Bad vprec mode: %s\n", env);
    exit(1);
  }
  if (verbose_mode) {
    print_debug("[MPFR] (info)", "mode = ", mode_names[mode]);
  }
  vprec_mode = mode;
  return vprec_mode;
}

long get_range() {
  if (vprec_range != -1) {
    return vprec_range;
  }

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
  if (verbose_mode) {
    print_debug("[MPFR] (info)", "range = ", env);
  }
  vprec_range = range;
  return vprec_range;
}

long get_precision() {
  if (vprec_precision != -1) {
    return vprec_precision;
  }

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

  if (verbose_mode) {
    print_debug_int("[MPFR] (info)", "precision = ", precision + 1);
  }

  vprec_precision = precision + 1;
  return vprec_precision;
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

void get_smallest_positive_subnormal_number(fptype_t type) {
  mpfr_exp_t _emin = mpfr_get_emin();
  mpfr_exp_t _emax = mpfr_get_emax();

  int range = get_range();
  int emax = (1 << (range - 1)) - 1;
  int emin = 1 - emax;
  // Precision withiout the implicit bit
  int precision = get_precision() - 1;

  mpfr_set_emin(mpfr_get_emin_min());
  mpfr_set_emax(mpfr_get_emax_max());

  mpfr_init2(smallest_subnormal, 256);
  /* 2 ^ (emin - precision)*/
  mpfr_set_ui_2exp(smallest_subnormal, 1, emin - precision, MPFR_RNDN);

  mpfr_set_emin(_emin);
  mpfr_set_emax(_emax);

  if (verbose_mode) {
    print_normalized_hex99_debug(
        "[MPFR] (info)", "smallest subnormal = ", smallest_subnormal, type);
  }
}

void get_largest_positive_normal_number(fptype_t type) {

  mpfr_exp_t _emin = mpfr_get_emin();
  mpfr_exp_t _emax = mpfr_get_emax();

  int range = get_range();
  int emax = (1 << (range - 1)) - 1;
  // Precision withiout the implicit bit
  int precision = get_precision() - 1;

  mpfr_set_emin(mpfr_get_emin_min());
  mpfr_set_emax(mpfr_get_emax_max());

  mpfr_init2(largest_normal, 256);

  /* 2^(-precision) */
  mpfr_set_ui_2exp(largest_normal, 1, -precision, MPFR_RNDN);
  /* 2 - 2^(-precision) */
  mpfr_ui_sub(largest_normal, 2, largest_normal, MPFR_RNDN);
  /* (2 - 2^(-precision)) * 2^(emax) */
  mpfr_mul_2ui(largest_normal, largest_normal, emax, MPFR_RNDN);

  mpfr_set_emin(_emin);
  mpfr_set_emax(_emax);

  if (verbose_mode) {
    print_normalized_hex99_debug("[MPFR] (info)",
                                 "largest normal = ", largest_normal, type);
  }
}

void apply_operation(mpfr_t res, mpfr_t a, mpfr_t b, const char op,
                     fptype_t type) {
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
  if (verbose_mode) {
    print_normalized_hex99_debug("[MPFR] (operation)", "res = ", res, type);
  }
  mpfr_subnormalize(res, i, MPFR_RNDN);
  if (verbose_mode) {
    print_normalized_hex99_debug("[MPFR] (subnormalize)", "res = ", res, type);
  }
}

void intermediate_rounding(mpfr_t x, mpfr_t intermediate,
                           mpfr_prec_t precision) {
  mpfr_clear_flags();
  int i = mpfr_prec_round(x, precision, MPFR_RNDN);
  mpfr_set(intermediate, x, MPFR_RNDN);

  i = mpfr_check_range(x, i, MPFR_RNDN);

  if (mpfr_cmpabs(x, largest_normal) > 0) {
    if (verbose_mode)
      fprintf(stderr, "Overflow detected\n");
    mpfr_set_inf(x, mpfr_sgn(x));
  } else if (mpfr_cmpabs(x, smallest_subnormal) < 0) {
    if (verbose_mode)
      fprintf(stderr, "Underflow detected\n");
    mpfr_set_zero(x, mpfr_sgn(x));
  }

  i = mpfr_subnormalize(x, i, MPFR_RNDN);
}

void compute(double a, double b, char op, fptype_t type) {
  const int working_precision = (type == float_type) ? 24 : 53;

  vprec_mode_t mode = get_mode();
  mpfr_prec_t precision = get_precision();
  mpfr_exp_t emax = get_emax();
  mpfr_exp_t emin = get_emin();

  mpfr_t ma, mb, mres, ma_inter, mb_inter, mres_inter;
  mpfr_inits2(working_precision, mres, ma, mb, ma_inter, mb_inter, mres_inter,
              (mpfr_ptr)0);

  if (type == float_type) {
    mpfr_set_flt(ma, a, MPFR_RNDN);
    mpfr_set_flt(mb, b, MPFR_RNDN);
  } else if (type == double_type) {
    mpfr_set_d(ma, a, MPFR_RNDN);
    mpfr_set_d(mb, b, MPFR_RNDN);
  }

  if (verbose_mode) {
    print_normalized_hex99_debug_2("[MPFR] (before rounding)",
                                   "a = ", "b = ", ma, mb, type);
  }

  if (mode == pb || mode == full) {
    mpfr_clear_flags();
    mpfr_set_emax(emax);
    mpfr_set_emin(emin);
    intermediate_rounding(ma, ma_inter, precision);
    intermediate_rounding(mb, mb_inter, precision);

    if (verbose_mode) {
      print_normalized_hex99_debug_2("[MPFR] (intermediate rounding)",
                                     "a = ", "b = ", ma_inter, mb_inter, type);
      print_normalized_hex99_debug_2("[MPFR] (subnormalized)",
                                     "a = ", "b = ", ma, mb, type);
    }
  }

  if (verbose_mode) {
    print_normalized_hex99_debug_2("[MPFR] (after rounding)",
                                   "a = ", "b = ", ma, mb, type);
  }

  mpfr_clear_flags();
  mpfr_set_default_prec(256);
  mpfr_set_emax(mpfr_get_emax_max());
  mpfr_set_emin(mpfr_get_emin_min());

  if (verbose_mode) {
    print_sep();
  }

  apply_operation(mres, ma, mb, op, type);

  if (mode == ob || mode == full) {
    mpfr_clear_flags();
    mpfr_set_emax(emax);
    mpfr_set_emin(emin);
    intermediate_rounding(mres, mres_inter, precision);

    if (verbose_mode) {
      print_normalized_hex99_debug("[MPFR] (intermediate rounding)",
                                   "res = ", mres_inter, type);
      print_normalized_hex99_debug("[MPFR] (subnormalized)", "res = ", mres,
                                   type);
    }
  }

  fprint_normalized_hex99_raw(mres, type, stdout, "\n");
}

void get_info(fptype_t type) {

  get_range();
  get_precision();
  get_mode();

  get_largest_positive_normal_number(type);
  get_smallest_positive_subnormal_number(type);

  if (verbose_mode) {
    print_sep();
  }
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

  fptype_t fptype = unknown_type;

  if (strcasecmp(type, "float") == 0) {
    fptype = float_type;
  } else if (strcasecmp(type, "double") == 0) {
    fptype = double_type;
  } else {
    fprintf(stderr, "Bad type: %s\n", type);
    exit(1);
  }

  verbose_mode = get_verbose();
  get_info(fptype);
  compute(a, b, op, fptype);
}