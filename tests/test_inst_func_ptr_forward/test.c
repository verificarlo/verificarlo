/* Regression test: pointer argument forwarding with --inst-func must not
 * trigger infinite recursion in getSizeOf.
 *
 * When a function takes a pointer argument and passes it to another function,
 * getSizeOf previously inspected users of the argument inside the function
 * rather than callers of the function, calling getSizeOf on itself indefinitely.
 */
#include <stdio.h>

__attribute__((noinline)) void helper(double *p) {
  p[0] += 1.0;
}

__attribute__((noinline)) void wrapper(double *p) {
  helper(p);
}

int main(void) {
  double arr[4] = {1.0, 2.0, 3.0, 4.0};
  wrapper(arr);
  printf("%.1f\n", arr[0]);
  return 0;
}
