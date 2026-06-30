#!/bin/bash
# Tests for the PRISM interflop backend.
#
# Validates:
#   1. VFC_BACKENDS="libinterflop_prism.so --precision-binary32 P32
#                                          --precision-binary64 P64"
#      sets the PRISM virtual precision at startup.
#   2. interflop_call(INTERFLOP_SET_PRECISION_BINARY32/64, V)
#      changes the PRISM virtual precision at runtime.

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
# Test 1: precision set via VFC_BACKENDS arguments
# ---------------------------------------------------------------------------
result=$(VFC_BACKENDS="libinterflop_prism.so --precision-binary32 10 --precision-binary64 30" \
         ./test 0 2>/dev/null)

p32=$(echo "$result" | grep "precision_binary32=" | cut -d= -f2)
p64=$(echo "$result" | grep "precision_binary64=" | cut -d= -f2)

if [ "$p32" != "10" ]; then
    echo "FAIL (env binary32): expected 10, got '$p32'"
    exit 1
fi
if [ "$p64" != "30" ]; then
    echo "FAIL (env binary64): expected 30, got '$p64'"
    exit 1
fi
echo "PASS: VFC_BACKENDS sets PRISM virtual precision"

# ---------------------------------------------------------------------------
# Test 2: default precision (no precision args)
# ---------------------------------------------------------------------------
result=$(VFC_BACKENDS="libinterflop_prism.so" ./test 0 2>/dev/null)

p32=$(echo "$result" | grep "precision_binary32=" | cut -d= -f2)
p64=$(echo "$result" | grep "precision_binary64=" | cut -d= -f2)
mode=$(echo "$result" | grep "mode=" | cut -d= -f2)

if [ "$p32" != "24" ]; then
    echo "FAIL (default binary32): expected 24, got '$p32'"
    exit 1
fi
if [ "$p64" != "53" ]; then
    echo "FAIL (default binary64): expected 53, got '$p64'"
    exit 1
fi
if [ "$mode" != "0" ]; then
    echo "FAIL (default mode): expected 0 (SR), got '$mode'"
    exit 1
fi
echo "PASS: default PRISM virtual precision and rounding mode are correct"

# ---------------------------------------------------------------------------
# Test 3: precision changed at runtime via interflop_call
# ---------------------------------------------------------------------------
result=$(VFC_BACKENDS="libinterflop_prism.so" ./test 1 2>/dev/null)

before_f32=$(echo "$result" | grep "before_f32=" | sed 's/.*before_f32=\([0-9]*\).*/\1/')
after_f32=$( echo "$result" | grep "after_f32="  | sed 's/.*after_f32=\([0-9]*\).*/\1/')
before_f64=$(echo "$result" | grep "before_f64=" | sed 's/.*before_f64=\([0-9]*\).*/\1/')
after_f64=$( echo "$result" | grep "after_f64="  | sed 's/.*after_f64=\([0-9]*\).*/\1/')
before_mode=$(echo "$result" | grep "before_mode=" | sed 's/.*before_mode=\([0-9]*\).*/\1/')
after_mode=$( echo "$result" | grep "after_mode="  | sed 's/.*after_mode=\([0-9]*\).*/\1/')

if [ "$before_f32" != "24" ]; then
    echo "FAIL (calluser initial binary32): expected 24, got '$before_f32'"
    exit 1
fi
if [ "$after_f32" != "5" ]; then
    echo "FAIL (calluser binary32): expected 5 after interflop_call, got '$after_f32'"
    exit 1
fi
if [ "$before_f64" != "53" ]; then
    echo "FAIL (calluser initial binary64): expected 53, got '$before_f64'"
    exit 1
fi
if [ "$after_f64" != "10" ]; then
    echo "FAIL (calluser binary64): expected 10 after interflop_call, got '$after_f64'"
    exit 1
fi
if [ "$before_mode" != "0" ]; then
    echo "FAIL (calluser initial mode): expected 0, got '$before_mode'"
    exit 1
fi
if [ "$after_mode" != "1" ]; then
    echo "FAIL (calluser mode): expected 1 after interflop_call, got '$after_mode'"
    exit 1
fi
echo "PASS: interflop_call changes PRISM virtual precision and rounding mode at runtime"

# ---------------------------------------------------------------------------
# Test 4: rounding mode set via VFC_BACKENDS string arguments (--mode rn)
# ---------------------------------------------------------------------------
result=$(VFC_BACKENDS="libinterflop_prism.so --mode rn" ./test 0 2>/dev/null)
mode=$(echo "$result" | grep "mode=" | cut -d= -f2)
if [ "$mode" != "1" ]; then
    echo "FAIL (env mode rn): expected 1, got '$mode'"
    exit 1
fi
echo "PASS: VFC_BACKENDS sets PRISM rounding mode to RN using string"

exit 0
