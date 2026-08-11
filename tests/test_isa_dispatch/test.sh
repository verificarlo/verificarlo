#!/bin/bash
# Test that the LLVM instrumentation pass emits ISA-specific vector wrapper
# calls when compiling with explicit architecture flags.
#
# For each ISA level the test:
#   1. Compiles test.c with the corresponding flag
#   2. Checks that the post-instrumentation IR (.2.ll) contains a call to the
#      expected ISA-specific wrapper (e.g. _4xfloatadd_avx2)

set -e

source ../paths.sh

new_env() {
    DIR=.$1
    rm -rf $DIR
    mkdir $DIR
    cp -r *.c $DIR
    cd $DIR
}

# Verify that the post-instrumentation IR contains the expected wrapper name.
check_wrapper() {
    local wrapper=$1
    if grep -q "$wrapper" *.2.ll; then
        echo "[PASS] Found $wrapper in IR"
    else
        echo "[FAIL] Expected $wrapper in IR but not found"
        echo "--- calls found in *.2.ll ---"
        grep "call " *.2.ll || true
        exit 1
    fi
}

# -------------------------------------------------------------------------
# x86_64 sub-tests
# -------------------------------------------------------------------------

# SSE2 baseline (-march=x86-64 keeps only the mandatory +sse2 features)
test_sse2() {
    new_env test_sse2
    verificarlo-c -c -march=x86-64 -emit-llvm --save-temps test.c
    check_wrapper "_4xfloatadd_sse2"
    check_wrapper "_2xdoubleadd_sse2"
}

# AVX only (-mavx does not imply AVX2)
test_avx() {
    new_env test_avx
    verificarlo-c -c -mavx -emit-llvm --save-temps test.c
    check_wrapper "_4xfloatadd_avx"
    check_wrapper "_2xdoubleadd_avx"
}

# AVX2
test_avx2() {
    new_env test_avx2
    verificarlo-c -c -mavx2 -emit-llvm --save-temps test.c
    check_wrapper "_4xfloatadd_avx2"
    check_wrapper "_2xdoubleadd_avx2"
}

# AVX-512F (checking the IR only — no need to run on AVX-512 hardware)
test_avx512() {
    new_env test_avx512
    verificarlo-c -c -mavx512f -emit-llvm --save-temps test.c
    check_wrapper "_4xfloatadd_avx512"
    check_wrapper "_2xdoubleadd_avx512"
}

# -------------------------------------------------------------------------
# AArch64 sub-tests
# -------------------------------------------------------------------------

# NEON (AArch64 baseline always has NEON)
test_neon() {
    new_env test_neon
    verificarlo-c -c -march=armv8-a -emit-llvm --save-temps test.c
    check_wrapper "_4xfloatadd_neon"
    check_wrapper "_2xdoubleadd_neon"
}

# SVE
test_sve() {
    new_env test_sve
    verificarlo-c -c -march=armv8-a+sve -emit-llvm --save-temps test.c
    check_wrapper "_4xfloatadd_sve"
    check_wrapper "_2xdoubleadd_sve"
}

# SVE2
test_sve2() {
    new_env test_sve2
    verificarlo-c -c -march=armv9-a+sve2 -emit-llvm --save-temps test.c
    check_wrapper "_4xfloatadd_sve2"
    check_wrapper "_2xdoubleadd_sve2"
}

# -------------------------------------------------------------------------
# Export helpers and dispatch by architecture
# -------------------------------------------------------------------------

export -f new_env check_wrapper

if [[ $(arch) == "x86_64" ]]; then
    export -f test_sse2 test_avx test_avx2 test_avx512
    parallel -k -j $(nproc) --halt now,fail=1 ::: test_sse2 test_avx test_avx2 test_avx512
elif [[ $(arch) == "aarch64" ]]; then
    export -f test_neon test_sve test_sve2
    parallel -k -j $(nproc) --halt now,fail=1 ::: test_neon test_sve test_sve2
else
    echo "Unsupported architecture: $(arch), skipping ISA dispatch test"
    exit 77
fi

echo "Test passed"
exit 0
