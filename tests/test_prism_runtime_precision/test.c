/* A runtime precision or rounding-mode change must reach the threads that are
 * executing instrumented arithmetic, not merely the getters.
 *
 * test_prism_backend_integration checks that interflop_call updates what the
 * PRISM getters report. That is necessary but not sufficient: precision and
 * rounding mode live in thread-local storage seeded from process-wide
 * defaults, so a setter can update the default -- and satisfy every getter --
 * while the threads doing the arithmetic keep rounding at the precision they
 * cached on first use. This test is PRISM-instrumented and checks the
 * arithmetic itself.
 *
 * The OpenMP region is the case that matters in practice: ATen dispatches
 * operators across a team whose threads have all executed arithmetic well
 * before any per-module precision hook runs. The change is made inside the
 * region, between two barriers, so the threads observing it are provably the
 * same ones that rounded before it.
 *
 * Usage:
 *   test serial   - one thread, arithmetic before and after the change
 *   test omp      - a team, arithmetic before and after the change
 *   test mode     - a team, rounding mode changed instead of precision
 */

#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "interflop/interflop.h"

#define MAX_THREADS 64

/* At t=24 the sum is exact in binary32; on the t=8 grid RN pulls it back to
 * 1.0. Both operands are volatile so the operation survives -O2 and is
 * instrumented rather than folded. */
static float inexact_sum(void) {
  volatile float a = 1.0F;
  volatile float b = 0x1.8p-10F;
  return a + b;
}

/* Repeating an inexact operation is bit-reproducible under RN and not under
 * SR, which is how the rounding mode is observed without inspecting state. */
static int is_deterministic(void) {
  const float first = inexact_sum();
  for (int i = 0; i < 64; i++) {
    if (inexact_sum() != first) {
      return 0;
    }
  }
  return 1;
}

static void run_serial(void) {
  const float before = inexact_sum();
  interflop_call(INTERFLOP_SET_PRECISION_BINARY32, 8);
  const float after = inexact_sum();
  printf("serial before=%.9g after=%.9g\n", before, after);
}

static void run_omp(void) {
  float before[MAX_THREADS];
  float after[MAX_THREADS];
  int used = 0;

  memset(before, 0, sizeof(before));
  memset(after, 0, sizeof(after));

#pragma omp parallel
  {
    const int id = omp_get_thread_num();
    if (id < MAX_THREADS) {
      /* Round once, so the thread caches the startup configuration. */
      before[id] = inexact_sum();
    }
#pragma omp master
    { used = omp_get_num_threads(); }

#pragma omp barrier
#pragma omp master
    { interflop_call(INTERFLOP_SET_PRECISION_BINARY32, 8); }
#pragma omp barrier

    if (id < MAX_THREADS) {
      after[id] = inexact_sum();
    }
  }

  if (used > MAX_THREADS) {
    used = MAX_THREADS;
  }
  printf("threads=%d\n", used);
  for (int i = 0; i < used; i++) {
    printf("thread=%d before=%.9g after=%.9g\n", i, before[i], after[i]);
  }
}

static void run_mode(void) {
  int deterministic[MAX_THREADS];
  int used = 0;

  memset(deterministic, 0, sizeof(deterministic));

#pragma omp parallel
  {
    const int id = omp_get_thread_num();
    (void)inexact_sum();
#pragma omp master
    { used = omp_get_num_threads(); }

#pragma omp barrier
#pragma omp master
    { interflop_call(INTERFLOP_SET_ROUNDING_MODE, 1); /* RN */ }
#pragma omp barrier

    if (id < MAX_THREADS) {
      deterministic[id] = is_deterministic();
    }
  }

  if (used > MAX_THREADS) {
    used = MAX_THREADS;
  }
  printf("threads=%d\n", used);
  for (int i = 0; i < used; i++) {
    printf("thread=%d deterministic=%d\n", i, deterministic[i]);
  }
}

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <serial|omp|mode>\n", argv[0]);
    return 1;
  }

  if (strcmp(argv[1], "serial") == 0) {
    run_serial();
  } else if (strcmp(argv[1], "omp") == 0) {
    run_omp();
  } else if (strcmp(argv[1], "mode") == 0) {
    run_mode();
  } else {
    fprintf(stderr, "Unknown case '%s'\n", argv[1]);
    return 1;
  }

  return 0;
}
