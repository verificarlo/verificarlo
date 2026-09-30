/* Regression test: the size --inst-func reports for a pointer argument that is
 * forwarded from the caller's own argument must hold for every caller.
 *
 * The size is fixed at compile time for a call site, but the backend reads and
 * rounds that many elements whichever caller supplied the pointer. It must
 * therefore be the smallest size over all callers, and 0 (unknown, the backend
 * skips the argument) as soon as one caller cannot be sized or may be hidden.
 */
#include <stdio.h>
#include <stdlib.h>

double gbuf[8];

__attribute__((noinline)) void helper(double *p) { p[0] += 1.0; }

/* Called with a heap buffer (unknown size) and with double[100]: 0 */
__attribute__((noinline)) void wrapper(double *p) { helper(p); }

/* Called with double[8] and double[100]: 8 */
__attribute__((noinline)) void only_arr(double *p) { helper(p); }

/* Also called through a function pointer, so some callers are hidden: 0 */
__attribute__((noinline)) void taken(double *p) { helper(p); }
void (*volatile taken_ptr)(double *) = taken;

/* Called with double[4] and by itself: the recursion must not turn it into 0.
 * The store after the call keeps -O2 from rewriting the recursion as a loop. */
__attribute__((noinline)) void rec(double *p, int n) {
  if (n > 0)
    rec(p, n - 1);
  p[0] *= 0.5;
}

int main(void) {
  double *heap = malloc(2 * sizeof(double));
  double arr[100] = {0};
  double arr4[4] = {0};

  heap[0] = heap[1] = 0.0;
  wrapper(heap);
  wrapper(arr);
  only_arr(gbuf);
  only_arr(arr);
  taken(arr4);
  taken_ptr(arr4);
  rec(arr4, 3);

  printf("%.1f %.1f %.1f %.1f\n", heap[0], arr[0], arr4[0], gbuf[0]);
  free(heap);
  return 0;
}
