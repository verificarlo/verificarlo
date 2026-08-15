/* Regression test: --inst-func on a function declaring a variable-length
 * array used to segfault.
 *
 * Clang brackets a VLA with a llvm.stacksave / llvm.stackrestore pair. The
 * function instrumentation pass moved the llvm.stackrestore call into an
 * out-of-line hook function, where it rewound the stack pointer past the
 * hook's own frame and destroyed its return address. The faulting instruction
 * pointer was the VLA length shifted left by 32 bits.
 */
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef __has_builtin
#define __has_builtin(x) 0
#endif

__attribute__((noinline)) double sum(const double f[], int n) {
  double s = 0.0;
  for (int i = 0; i < n; i++) {
    s += f[i];
  }
  return s;
}

/* A variadic callee, so that the llvm.va_* intrinsics are exercised too: they
 * are just as frame-sensitive as llvm.stackrestore. */
__attribute__((noinline)) double vsum(int n, ...) {
  va_list ap;
  va_start(ap, n);
  double s = 0.0;
  for (int i = 0; i < n; i++) {
    s += va_arg(ap, double);
  }
  va_end(ap);
  return s;
}

/* Lowered to llvm.fma.f64 at -O2. Floating-point intrinsics must keep being
 * instrumented: the fix must not block intrinsics wholesale. */
__attribute__((noinline)) double fmatest(double a, double b, double c) {
  return fma(a, b, c);
}

/* A void pointer with conflicting uses must be classified from each call site,
 * not from the first use found while walking the callee body. */
__attribute__((noinline)) double read_mixed(const void *p, int as_double) {
  if (as_double) {
    return *(const double *)p;
  }
  return (double)*(const float *)p;
}

#if __has_builtin(__builtin_isfpclass)
__attribute__((noinline)) int fpclasstest(double x) {
  return __builtin_isfpclass(x, 1 << 8);
}
#endif

int main(int argc, char **argv) {
  int n = (argc > 1) ? atoi(argv[1]) : 100;

  double f[n]; /* VLA -- the trigger */
  for (int i = 0; i < n; i++) {
    f[i] = 1.0 / (double)(i + 1);
  }

  printf("%.16e\n", sum(f, n));
  printf("%.16e\n", vsum(3, 1.0, 2.0, 3.0));
  printf("%.16e\n", fmatest(2.0, 3.0, 4.0));

  double mixed_d = 2.5;
  float mixed_f = 1.25F;
  printf("%.16e\n", read_mixed(&mixed_d, 1));
  printf("%.16e\n", read_mixed(&mixed_f, 0));

#if __has_builtin(__builtin_isfpclass)
  printf("%d\n", fpclasstest(sum(f, n)));
#endif
  return 0;
}
