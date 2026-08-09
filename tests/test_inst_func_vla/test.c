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

int main(int argc, char **argv) {
  int n = (argc > 1) ? atoi(argv[1]) : 100;

  double f[n]; /* VLA -- the trigger */
  for (int i = 0; i < n; i++) {
    f[i] = 1.0 / (double)(i + 1);
  }

  printf("%.16e\n", sum(f, n));
  printf("%.16e\n", vsum(3, 1.0, 2.0, 3.0));
  printf("%.16e\n", fmatest(2.0, 3.0, 4.0));
  return 0;
}
