#!/bin/bash
# interflop_call(INTERFLOP_ROUND_DW_ID, ...) rounds a double-word number x + e
# with the PRISM backend.
#
# Validates:
#   1. RN at reduced precision: the low part e breaks a tie of x on the
#      virtual grid, so the call rounds x + e and not x alone.
#   2. SR in binary64 and binary32: x + ulp/4 rounds up with probability 1/4.
#   3. --seed makes the rounded sequence reproducible.

set -e

source "$(dirname "$0")/../paths.sh"

if [ "${BUILD_PRISM}" = "no" ]; then
    echo "this test is not run when using --without-prism"
    # Exit with 77 to mark the test skipped
    exit 77
fi

export VFC_BACKENDS_LOGGER=False

make --silent PRISM_BACKEND=sr

# ---------------------------------------------------------------------------
# Test 1: RN at t = 10, x = 1 + 2^-10 is a tie
# ---------------------------------------------------------------------------
result=$(VFC_BACKENDS="libinterflop_prism.so --mode rn --precision-binary64 10" ./test tie)
expected="down=0x1p+0 up=0x1.008p+0"
if [ "$result" != "$expected" ]; then
    echo "FAIL (rn tie): expected '$expected', got '$result'"
    exit 1
fi
echo "PASS: RN rounds x + e, the low part breaking a tie of x"

# ---------------------------------------------------------------------------
# Test 2: SR, P(up) = 1/4 for x = 1, e = ulp/4
# ---------------------------------------------------------------------------
# N = 10000, p = 0.25: mean 2500, standard deviation ~43.
N=10000
for type in sr64 sr32; do
    result=$(VFC_BACKENDS="libinterflop_prism.so --mode sr" ./test $type $N)
    up=$(echo "$result" | sed -n 's/^up=\([0-9]*\)$/\1/p')
    if [ -z "$up" ]; then
        echo "FAIL ($type): got '$result'"
        exit 1
    fi
    if [ "$up" -lt 2200 ] || [ "$up" -gt 2800 ]; then
        echo "FAIL ($type): $up/$N rounded up, expected about 2500"
        exit 1
    fi
    echo "PASS: SR $type rounds x + ulp/4 up $up/$N times"
done

# ---------------------------------------------------------------------------
# Test 3: the seed fixes the sequence
# ---------------------------------------------------------------------------
run1=$(VFC_BACKENDS="libinterflop_prism.so --mode sr --seed 42" ./test seq 256)
run2=$(VFC_BACKENDS="libinterflop_prism.so --mode sr --seed 42" ./test seq 256)
run3=$(VFC_BACKENDS="libinterflop_prism.so --mode sr --seed 43" ./test seq 256)
if [ "$run1" != "$run2" ]; then
    echo "FAIL (seed): two runs with --seed 42 differ"
    exit 1
fi
if [ "$run1" = "$run3" ]; then
    echo "FAIL (seed): runs with --seed 42 and --seed 43 are identical"
    exit 1
fi
echo "PASS: --seed makes the sequence reproducible"

exit 0
