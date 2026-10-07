#!/bin/bash
# Test that loops auto-vectorized for SVE on AArch64 are instrumented (MCA
# and PRISM), whether the loop vectorizer chooses fixed-width or scalable
# vectors, and when scalable vectors are forced. The results of scalable
# vector operations are checked in test_sve_scalable.

source ../paths.sh

if [[ $(arch) != "aarch64" ]]; then
    echo "Not an AArch64 host, skipping SVE vectorization test"
    exit 77
fi

SVE_FLAGS="-O3 -march=armv8-a+sve"
SAMPLES=20

# Does this CPU run SVE code? (compile-time checks run on any AArch64 host)
if grep -qw sve /proc/cpuinfo; then
    HAS_SVE=1
else
    HAS_SVE=0
fi

new_env() {
    DIR=.$1
    rm -rf $DIR
    mkdir $DIR
    cp test.c $DIR
    cd $DIR
}

# No floating-point arithmetic on scalable vectors may be left native
check_no_native_scalable() {
    if grep -qE "= f(add|sub|mul|div) [a-z ]*<vscale x" *.2.ll; then
        echo "[FAIL] Native scalable vector operations left in the IR"
        grep -E "= f(add|sub|mul|div) [a-z ]*<vscale x" *.2.ll | head -3
        exit 1
    fi
    echo "[PASS] No native scalable vector operations"
}

check_variability() {
    local backend=$1
    if [[ $HAS_SVE == 0 ]]; then
        echo "[SKIP] CPU has no SVE, not running the binary"
        return
    fi
    rm -f out
    for i in $(seq 1 $SAMPLES); do
        VFC_BACKENDS_LOGGER=False VFC_BACKENDS="$backend" ./test >>out
    done
    if [[ $(sort -u out | wc -l) -le 1 ]]; then
        echo "[FAIL] No variability with $backend"
        sort -u out
        exit 1
    fi
    echo "[PASS] Variability with $backend"
}

test_mca() {
    new_env mca
    verificarlo-c $SVE_FLAGS --save-temps test.c -o test || {
        echo "[FAIL] MCA compilation failed"
        exit 1
    }
    check_no_native_scalable
    check_variability "libinterflop_mca.so --precision-binary32=12 --precision-binary64=30"
}

test_prism() {
    if [[ "${BUILD_PRISM}" == "no" ]]; then
        echo "[SKIP] PRISM not built"
        return
    fi
    new_env prism
    verificarlo-c --prism-backend=sr $SVE_FLAGS --save-temps test.c -o test || {
        echo "[FAIL] PRISM compilation failed"
        exit 1
    }
    check_no_native_scalable
    check_variability "libinterflop_prism.so"
}

# Scalable vectors forced by the user
test_forced_scalable() {
    new_env forced
    verificarlo-c $SVE_FLAGS -mllvm -scalable-vectorization=on --save-temps \
        test.c -o test || {
        echo "[FAIL] Compilation with forced scalable vectors failed"
        exit 1
    }
    if ! grep -q "<vscale x" *.1.ll; then
        echo "[FAIL] Expected scalable vectors in the IR"
        exit 1
    fi
    check_no_native_scalable
    check_variability "libinterflop_mca.so --precision-binary32=12 --precision-binary64=30"
}

(test_mca) || exit 1
(test_prism) || exit 1
(test_forced_scalable) || exit 1

echo "Test passed"
exit 0
