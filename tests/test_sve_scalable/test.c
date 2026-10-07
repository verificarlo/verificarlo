/*
 * test_sve_scalable/test.c
 *
 * Floating-point operations on scalable vectors (<vscale x N x T>, SVE):
 *   - loops vectorized with scalable vectors: add, sub, mul, div, fma,
 *     a*b+c (llvm.fmuladd), comparisons, and strict (ordered) sums;
 *   - predicated SVE intrinsics (ACLE), with a partial predicate.
 *
 * Each kernel is computed twice: with scalable vectors, and with scalar code
 * applying the same operations to each lane. With a deterministic backend
 * (IEEE, VPREC), both must give the same bits, for any SVE vector length.
 *
 * Output, one line per kernel:
 *   <kernel> <hash of the results> active=<OK|MISMATCH> inactive=<OK|MISMATCH>
 * "active" compares the lanes computed by the kernel with the scalar code,
 * "inactive" checks that the lanes left out by the predicate have the value
 * required by the intrinsic (first data operand for _m, zero for _z).
 *
 * Usage: ./test [inexact|exact|count]
 *   inexact (default): random inputs, most results are inexact
 *   exact: small integers, so that every result is exact
 *   count: a single svadd_m on one vector, see test.sh
 */
#include <arm_sve.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define N 1003

#define VECTORIZE                                                              \
  _Pragma(                                                                     \
      "clang loop vectorize(enable) vectorize_width(4, scalable) interleave_count(1)")
#define SCALAR _Pragma("clang loop vectorize(disable) interleave(disable)")
#define NOINLINE __attribute__((noinline))

static double xd[N], yd[N], zd[N], rd[N], refd[N];
static float xf[N], yf[N], zf[N], rf[N], reff[N];

static uint64_t hash(const void *data, size_t size) {
  const unsigned char *bytes = data;
  uint64_t h = 1469598103934665603ULL;
  for (size_t i = 0; i < size; i++)
    h = (h ^ bytes[i]) * 1099511628211ULL;
  return h;
}

/* Random double in [2^e, 2^(e+1)), built from its bits: the inputs must not
 * depend on the backend, so they are computed without floating-point
 * arithmetic. */
static uint64_t seed = 42;
static double random_double(int e) {
  seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
  uint64_t bits = ((uint64_t)(1023 + e) << 52) | (seed >> 12);
  double d;
  memcpy(&d, &bits, sizeof(d));
  return d;
}

static void init(int exact) {
  for (int i = 0; i < N; i++) {
    if (exact) {
      // small integers, and powers of two for x and y, which are used as
      // divisors: every result is exact
      xd[i] = (double)((i % 2 ? -1 : 1) * (1 << (i % 7)));
      yd[i] = (double)(1 << (i % 5));
      zd[i] = (double)(i % 13 - 6);
    } else {
      // different exponents, so that sums and differences are inexact
      xd[i] = random_double(0);
      yd[i] = random_double(-3);
      zd[i] = random_double(1);
    }
    xf[i] = (float)xd[i];
    yf[i] = (float)yd[i];
    zf[i] = (float)zd[i];
  }
}

static void report(const char *name, const void *results, size_t size,
                   int active_ok, int inactive_ok) {
  printf("%-14s %016llx active=%s inactive=%s\n", name,
         (unsigned long long)hash(results, size),
         active_ok ? "OK" : "MISMATCH", inactive_ok ? "OK" : "MISMATCH");
}

/* Loops vectorized with scalable vectors, and their scalar reference */

#define DEFINE_LOOP(T, S, NAME, EXPR)                                          \
  NOINLINE void vec_##NAME##_##S(int n, T *restrict r, const T *restrict x,    \
                                 const T *restrict y, const T *restrict z) {   \
    VECTORIZE for (int i = 0; i < n; i++) r[i] = EXPR;                         \
  }                                                                            \
  NOINLINE void ref_##NAME##_##S(int n, T *restrict r, const T *restrict x,    \
                                 const T *restrict y, const T *restrict z) {   \
    SCALAR for (int i = 0; i < n; i++) r[i] = EXPR;                            \
  }

#define DEFINE_LOOPS(T, S, FMA)                                                \
  DEFINE_LOOP(T, S, add, x[i] + y[i])                                          \
  DEFINE_LOOP(T, S, sub, x[i] - y[i])                                          \
  DEFINE_LOOP(T, S, mul, x[i] * y[i])                                          \
  DEFINE_LOOP(T, S, div, x[i] / y[i])                                          \
  DEFINE_LOOP(T, S, fma, FMA(x[i], y[i], z[i]))                                \
  DEFINE_LOOP(T, S, muladd, x[i] * y[i] + z[i])                                \
  DEFINE_LOOP(T, S, cmp, (T)(x[i] < y[i]))                                     \
  /* No pragma: forcing vectorization allows reordering the sum, while the */ \
  /* default on SVE is a strict (ordered) reduction on scalable vectors */    \
  NOINLINE T vec_sum_##S(int n, const T *x) {                                  \
    T s = 0;                                                                   \
    for (int i = 0; i < n; i++)                                                \
      s += x[i];                                                               \
    return s;                                                                  \
  }                                                                            \
  NOINLINE T ref_sum_##S(int n, const T *x) {                                  \
    T s = 0;                                                                   \
    SCALAR for (int i = 0; i < n; i++) s += x[i];                              \
    return s;                                                                  \
  }

DEFINE_LOOPS(double, f64, __builtin_fma)
DEFINE_LOOPS(float, f32, __builtin_fmaf)

#define CHECK_LOOP(T, S, NAME, X, Y, Z, R, REF)                                \
  do {                                                                         \
    vec_##NAME##_##S(N, R, X, Y, Z);                                           \
    ref_##NAME##_##S(N, REF, X, Y, Z);                                         \
    report(#NAME "_" #S, R, sizeof(R), !memcmp(R, REF, sizeof(R)), 1);         \
  } while (0)

/* Sums of the first N - k elements, k < 32: the results of stochastic
 * rounding vary for each sum, and so do the partial last vectors */
#define CHECK_REDUCTION(T, S, NAME, X)                                         \
  do {                                                                         \
    T v[32], r[32];                                                            \
    for (int k = 0; k < 32; k++) {                                             \
      v[k] = vec_##NAME##_##S(N - k, X);                                       \
      r[k] = ref_##NAME##_##S(N - k, X);                                       \
    }                                                                          \
    report(#NAME "_" #S, v, sizeof(v), !memcmp(v, r, sizeof(v)), 1);          \
  } while (0)

#define CHECK_LOOPS(T, S, X, Y, Z, R, REF)                                     \
  CHECK_LOOP(T, S, add, X, Y, Z, R, REF);                                      \
  CHECK_LOOP(T, S, sub, X, Y, Z, R, REF);                                      \
  CHECK_LOOP(T, S, mul, X, Y, Z, R, REF);                                      \
  CHECK_LOOP(T, S, div, X, Y, Z, R, REF);                                      \
  CHECK_LOOP(T, S, fma, X, Y, Z, R, REF);                                      \
  CHECK_LOOP(T, S, muladd, X, Y, Z, R, REF);                                   \
  CHECK_LOOP(T, S, cmp, X, Y, Z, R, REF);                                      \
  CHECK_REDUCTION(T, S, sum, X)

/* Predicated SVE intrinsics.
 *
 * The predicate keeps the even lanes of each vector (and stops at N), so
 * lane i of the arrays is active iff i is even. Inactive lanes take the
 * first data operand (_m), zero (_z), or are undefined (_x, not checked). */

enum inactive { FIRST, ZERO, UNDEFINED };

enum op {
  ADD, SUB, SUBR, MUL, DIV, DIVR, // (pg, x, y)
  MLA, MLS, NMLA, NMLS,           // (pg, z, x, y): z is the accumulator
  MAD, MSB, NMAD, NMSB,           // (pg, x, y, z)
};

#define DEFINE_REF(T, S, FMA)                                                  \
  NOINLINE T ref_op_##S(enum op op, T x, T y, T z) {                           \
    switch (op) {                                                              \
    case ADD:                                                                  \
      return x + y;                                                            \
    case SUB:                                                                  \
      return x - y;                                                            \
    case SUBR:                                                                 \
      return y - x;                                                            \
    case MUL:                                                                  \
      return x * y;                                                            \
    case DIV:                                                                  \
      return x / y;                                                            \
    case DIVR:                                                                 \
      return y / x;                                                            \
    case MLA:                                                                  \
    case MAD:                                                                  \
      return FMA(x, y, z);                                                     \
    case MLS:                                                                  \
    case MSB:                                                                  \
      return FMA(-x, y, z);                                                    \
    case NMLA:                                                                 \
    case NMAD:                                                                 \
      return FMA(-x, y, -z);                                                   \
    case NMLS:                                                                 \
    case NMSB:                                                                 \
      return FMA(x, y, -z);                                                    \
    }                                                                          \
    return 0;                                                                  \
  }

DEFINE_REF(double, f64, __builtin_fma)
DEFINE_REF(float, f32, __builtin_fmaf)

/* lane i is active iff i is even, see svdupq below */
#define DEFINE_ACLE(T, V, S, B, CNT, ALT, NAME, OP, INACTIVE, CALL)            \
  NOINLINE void acle_##NAME##_##S(int n, T *r, const T *xs, const T *ys,       \
                                  const T *zs) {                               \
    for (int i = 0; i < n; i += (int)CNT()) {                                  \
      svbool_t in = svwhilelt_##B##_s32(i, n);                                 \
      svbool_t pg = svand_z(in, in, ALT);                                      \
      V x = svld1(in, xs + i), y = svld1(in, ys + i), z = svld1(in, zs + i);   \
      svst1(in, r + i, CALL);                                                  \
    }                                                                          \
  }                                                                            \
  static void check_##NAME##_##S(const T *xs, const T *ys, const T *zs,        \
                                 T *r, T *ref) {                               \
    acle_##NAME##_##S(N, r, xs, ys, zs);                                       \
    int active_ok = 1, inactive_ok = 1;                                        \
    for (int i = 0; i < N; i++) {                                              \
      /* MLA-like ops take the accumulator first: (pg, z, x, y) */             \
      int acc_first = OP == MLA || OP == MLS || OP == NMLA || OP == NMLS;      \
      T first = acc_first ? zs[i] : xs[i];                                     \
      if (i % 2 == 0) {                                                        \
        ref[i] = ref_op_##S(OP, xs[i], ys[i], zs[i]);                          \
        active_ok &= !memcmp(&r[i], &ref[i], sizeof(T));                       \
      } else if (INACTIVE == FIRST) {                                          \
        inactive_ok &= !memcmp(&r[i], &first, sizeof(T));                      \
      } else if (INACTIVE == ZERO) {                                           \
        T zero = 0;                                                            \
        inactive_ok &= !memcmp(&r[i], &zero, sizeof(T));                       \
      } else {                                                                 \
        r[i] = 0; /* undefined, excluded from the hash */                      \
      }                                                                        \
    }                                                                          \
    report("sv" #NAME "_" #S, r, N * sizeof(T), active_ok, inactive_ok);       \
  }

#define DEFINE_ACLES(T, V, S, B, CNT, ALT)                                     \
  DEFINE_ACLE(T, V, S, B, CNT, ALT, add_m, ADD, FIRST, svadd_m(pg, x, y))              \
  DEFINE_ACLE(T, V, S, B, CNT, ALT, add_x, ADD, UNDEFINED, svadd_x(pg, x, y))          \
  DEFINE_ACLE(T, V, S, B, CNT, ALT, sub_m, SUB, FIRST, svsub_m(pg, x, y))              \
  DEFINE_ACLE(T, V, S, B, CNT, ALT, subr_m, SUBR, FIRST, svsubr_m(pg, x, y))           \
  DEFINE_ACLE(T, V, S, B, CNT, ALT, mul_m, MUL, FIRST, svmul_m(pg, x, y))              \
  DEFINE_ACLE(T, V, S, B, CNT, ALT, mul_x, MUL, UNDEFINED, svmul_x(pg, x, y))          \
  DEFINE_ACLE(T, V, S, B, CNT, ALT, mul_z, MUL, ZERO, svmul_z(pg, x, y))               \
  DEFINE_ACLE(T, V, S, B, CNT, ALT, div_m, DIV, FIRST, svdiv_m(pg, x, y))              \
  DEFINE_ACLE(T, V, S, B, CNT, ALT, div_z, DIV, ZERO, svdiv_z(pg, x, y))               \
  DEFINE_ACLE(T, V, S, B, CNT, ALT, divr_m, DIVR, FIRST, svdivr_m(pg, x, y))           \
  DEFINE_ACLE(T, V, S, B, CNT, ALT, mla_m, MLA, FIRST, svmla_m(pg, z, x, y))           \
  DEFINE_ACLE(T, V, S, B, CNT, ALT, mls_m, MLS, FIRST, svmls_m(pg, z, x, y))           \
  DEFINE_ACLE(T, V, S, B, CNT, ALT, nmla_m, NMLA, FIRST, svnmla_m(pg, z, x, y))        \
  DEFINE_ACLE(T, V, S, B, CNT, ALT, nmls_m, NMLS, FIRST, svnmls_m(pg, z, x, y))        \
  DEFINE_ACLE(T, V, S, B, CNT, ALT, mad_m, MAD, FIRST, svmad_m(pg, x, y, z))           \
  DEFINE_ACLE(T, V, S, B, CNT, ALT, msb_m, MSB, FIRST, svmsb_m(pg, x, y, z))           \
  DEFINE_ACLE(T, V, S, B, CNT, ALT, nmad_m, NMAD, FIRST, svnmad_m(pg, x, y, z))        \
  DEFINE_ACLE(T, V, S, B, CNT, ALT, nmsb_m, NMSB, FIRST, svnmsb_m(pg, x, y, z))

DEFINE_ACLES(double, svfloat64_t, f64, b64, svcntd, svdupq_n_b64(1, 0))
DEFINE_ACLES(float, svfloat32_t, f32, b32, svcntw, svdupq_n_b32(1, 0, 1, 0))

#define CHECK_ACLES(S, X, Y, Z, R, REF)                                        \
  check_add_m_##S(X, Y, Z, R, REF);                                            \
  check_add_x_##S(X, Y, Z, R, REF);                                            \
  check_sub_m_##S(X, Y, Z, R, REF);                                            \
  check_subr_m_##S(X, Y, Z, R, REF);                                           \
  check_mul_m_##S(X, Y, Z, R, REF);                                            \
  check_mul_x_##S(X, Y, Z, R, REF);                                            \
  check_mul_z_##S(X, Y, Z, R, REF);                                            \
  check_div_m_##S(X, Y, Z, R, REF);                                            \
  check_div_z_##S(X, Y, Z, R, REF);                                            \
  check_divr_m_##S(X, Y, Z, R, REF);                                           \
  check_mla_m_##S(X, Y, Z, R, REF);                                            \
  check_mls_m_##S(X, Y, Z, R, REF);                                            \
  check_nmla_m_##S(X, Y, Z, R, REF);                                           \
  check_nmls_m_##S(X, Y, Z, R, REF);                                           \
  check_mad_m_##S(X, Y, Z, R, REF);                                            \
  check_msb_m_##S(X, Y, Z, R, REF);                                            \
  check_nmad_m_##S(X, Y, Z, R, REF);                                           \
  check_nmsb_m_##S(X, Y, Z, R, REF)

/* One svadd_m on the first vector of xd: the backend must be called once
 * per active lane, i.e. svcntd() / 2 times. */
NOINLINE svfloat64_t count_add(svbool_t pg, svfloat64_t x, svfloat64_t y) {
  return svadd_m(pg, x, y);
}

int main(int argc, char *argv[]) {
  const char *mode = argc > 1 ? argv[1] : "inexact";

  if (!strcmp(mode, "count")) {
    // no floating-point arithmetic besides count_add
    for (int i = 0; i < N; i++) {
      xd[i] = (double)i;
      yd[i] = (double)(N - i);
    }
    svbool_t all = svptrue_b64();
    svbool_t pg = svdupq_n_b64(1, 0);
    svfloat64_t r = count_add(pg, svld1(all, xd), svld1(all, yd));
    svst1(all, rd, r);
    printf("active lanes: %llu\n", (unsigned long long)svcntp_b64(all, pg));
    return 0;
  }

  init(!strcmp(mode, "exact"));
  CHECK_LOOPS(double, f64, xd, yd, zd, rd, refd);
  CHECK_LOOPS(float, f32, xf, yf, zf, rf, reff);
  CHECK_ACLES(f64, xd, yd, zd, rd, refd);
  CHECK_ACLES(f32, xf, yf, zf, rf, reff);
  return 0;
}
