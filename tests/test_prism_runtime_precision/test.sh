#!/bin/bash
# A runtime precision or rounding-mode change must reach the threads running
# instrumented arithmetic.
#
# Validates:
#   1. interflop_call(INTERFLOP_SET_PRECISION_BINARY32, 8) changes what the
#      calling thread computes, after that thread has already rounded.
#   2. The same change, made inside an OpenMP region, reaches every worker of
#      the team -- the ATen case.
#   3. interflop_call(INTERFLOP_SET_ROUNDING_MODE, RN) likewise reaches every
#      worker, observed through the bit-reproducibility of RN.

set -e

source "$(dirname "$0")/../paths.sh"

if [ "${BUILD_PRISM}" = "no" ]; then
    echo "this test is not run when using --without-prism"
    # Exit with 77 to mark the test skipped
    exit 77
fi

export VFC_BACKENDS_LOGGER=False
export OMP_NUM_THREADS=4

# 1.0 + 1.5 * 2^-10, exact in binary32; on the t=8 grid RN gives 1.0.
EXACT="1.00146484"
ROUNDED="1"

make --silent PRISM_BACKEND=sr

# ---------------------------------------------------------------------------
# Test 1: the calling thread observes its own change
# ---------------------------------------------------------------------------
result=$(VFC_BACKENDS="libinterflop_prism.so --mode rn" ./test serial 2>/dev/null)

before=$(echo "$result" | sed 's/.*before=\([^ ]*\).*/\1/')
after=$(echo "$result" | sed 's/.*after=\([^ ]*\)$/\1/')

if [ "${before:0:10}" != "$EXACT" ]; then
    echo "FAIL (serial before): expected $EXACT, got '$before'"
    exit 1
fi
if [ "$after" != "$ROUNDED" ]; then
    echo "FAIL (serial after): precision change did not reach the arithmetic;"
    echo "                     expected $ROUNDED, got '$after'"
    exit 1
fi
echo "PASS: a precision change reaches the calling thread's arithmetic"

# ---------------------------------------------------------------------------
# Test 2: every worker of the team observes it
# ---------------------------------------------------------------------------
result=$(VFC_BACKENDS="libinterflop_prism.so --mode rn" ./test omp 2>/dev/null)

threads=$(echo "$result" | grep '^threads=' | cut -d= -f2)
if [ "$threads" -lt 2 ]; then
    echo "FAIL: expected an OpenMP team of at least 2 threads, got '$threads'"
    exit 1
fi

while read -r line; do
    [ -z "$line" ] && continue
    id=$(echo "$line" | sed 's/thread=\([0-9]*\).*/\1/')
    before=$(echo "$line" | sed 's/.*before=\([^ ]*\).*/\1/')
    after=$(echo "$line" | sed 's/.*after=\([^ ]*\)$/\1/')

    if [ "${before:0:10}" != "$EXACT" ]; then
        echo "FAIL (omp before, thread $id): expected $EXACT, got '$before'"
        exit 1
    fi
    if [ "$after" != "$ROUNDED" ]; then
        echo "FAIL (omp after, thread $id): worker kept rounding at the"
        echo "                              precision it cached; expected"
        echo "                              $ROUNDED, got '$after'"
        exit 1
    fi
done < <(echo "$result" | grep '^thread=')

echo "PASS: a precision change reaches every worker of an OpenMP team ($threads threads)"

# ---------------------------------------------------------------------------
# Test 3: the rounding mode reaches every worker
# ---------------------------------------------------------------------------
result=$(VFC_BACKENDS="libinterflop_prism.so" ./test mode 2>/dev/null)

threads=$(echo "$result" | grep '^threads=' | cut -d= -f2)
if [ "$threads" -lt 2 ]; then
    echo "FAIL: expected an OpenMP team of at least 2 threads, got '$threads'"
    exit 1
fi

while read -r line; do
    [ -z "$line" ] && continue
    id=$(echo "$line" | sed 's/thread=\([0-9]*\).*/\1/')
    det=$(echo "$line" | sed 's/.*deterministic=\([0-9]*\)$/\1/')
    if [ "$det" != "1" ]; then
        echo "FAIL (mode, thread $id): worker still rounding stochastically"
        echo "                         after the switch to RN"
        exit 1
    fi
done < <(echo "$result" | grep '^thread=')

echo "PASS: a rounding-mode change reaches every worker of an OpenMP team ($threads threads)"

exit 0
