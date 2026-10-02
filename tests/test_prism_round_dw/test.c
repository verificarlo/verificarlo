/* interflop_call(INTERFLOP_ROUND_DW_ID, ...) must round the double-word
 * number x + e with PRISM's mode, virtual precision and random state.
 *
 * Usage:
 *   test tie      - x = 1 + 2^-10 is a tie at t = 10; prints the result for
 *                   e = -2^-60 and e = +2^-60 (run with --mode rn)
 *   test sr64 N   - N roundings of x = 1, e = 2^-54 (a quarter ulp); prints
 *                   how many rounded up
 *   test sr32 N   - same in binary32 with x = 1, e = 2^-25
 *   test seq N    - prints N roundings of x = 1, e = 2^-54 as 0 (down) or 1
 *                   (up), to compare runs with the same seed
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "interflop/interflop.h"

static double round_dw64(double x, double e) {
  interflop_call(INTERFLOP_ROUND_DW_ID, FDOUBLE, &x, &e);
  return x;
}

static float round_dw32(float x, float e) {
  interflop_call(INTERFLOP_ROUND_DW_ID, FFLOAT, &x, &e);
  return x;
}

static void run_tie(void) {
  const double x = 0x1.004p+0; /* 1 + 2^-10 */
  printf("down=%a up=%a\n", round_dw64(x, -0x1p-60), round_dw64(x, 0x1p-60));
}

static void run_sr64(int n) {
  int up = 0;
  for (int i = 0; i < n; i++) {
    const double r = round_dw64(1.0, 0x1p-54);
    if (r == 0x1.0000000000001p+0) {
      up++;
    } else if (r != 1.0) {
      printf("unexpected=%a\n", r);
      return;
    }
  }
  printf("up=%d\n", up);
}

static void run_sr32(int n) {
  int up = 0;
  for (int i = 0; i < n; i++) {
    const float r = round_dw32(1.0F, 0x1p-25F);
    if (r == 0x1.000002p+0F) {
      up++;
    } else if (r != 1.0F) {
      printf("unexpected=%a\n", (double)r);
      return;
    }
  }
  printf("up=%d\n", up);
}

static void run_seq(int n) {
  for (int i = 0; i < n; i++) {
    putchar(round_dw64(1.0, 0x1p-54) == 1.0 ? '0' : '1');
  }
  putchar('\n');
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "usage: %s tie|sr64 N|sr32 N|seq N\n", argv[0]);
    return 1;
  }
  const int n = (argc > 2) ? atoi(argv[2]) : 0;
  if (strcmp(argv[1], "tie") == 0) {
    run_tie();
  } else if (strcmp(argv[1], "sr64") == 0) {
    run_sr64(n);
  } else if (strcmp(argv[1], "sr32") == 0) {
    run_sr32(n);
  } else if (strcmp(argv[1], "seq") == 0) {
    run_seq(n);
  } else {
    fprintf(stderr, "unknown mode: %s\n", argv[1]);
    return 1;
  }
  return 0;
}
