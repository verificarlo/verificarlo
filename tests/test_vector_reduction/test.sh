#!/bin/bash
# Test that the arithmetic of vectorized floating-point reductions
# (llvm.vector.reduce.fadd/fmul) is instrumented:
#   - no fixed-width reduction intrinsic is left in the instrumented IR,
#   - MCA and PRISM results vary across runs,
#   - the IEEE backend reproduces the native result bit for bit (the
#     expansion keeps the strict, in-order semantics).

source ../paths.sh

SAMPLES=20

new_env() {
    DIR=.$1
    rm -rf $DIR
    mkdir $DIR
    cp test.c $DIR
    cd $DIR
}

REDUCTION="@llvm\.vector\.reduce\.f(add|mul)\.v[0-9]+f(32|64)"

check_no_reduction_left() {
    if grep -qE "call .*${REDUCTION}" *.2.ll; then
        echo "[FAIL] Vector reduction left uninstrumented ($1)"
        grep -E "call .*${REDUCTION}" *.2.ll | head -3
        exit 1
    fi
    echo "[PASS] No vector reduction left ($1)"
}

check_variability() {
    local name=$1
    local backend=$2
    rm -f out
    for i in $(seq 1 $SAMPLES); do
        VFC_BACKENDS_LOGGER=False VFC_BACKENDS="$backend" ./test >>out
    done
    if [[ $(sort -u out | wc -l) -le 1 ]]; then
        echo "[FAIL] No variability ($name)"
        sort -u out
        exit 1
    fi
    echo "[PASS] Variability ($name)"
}

# flags, label, expect reductions in the input IR (1/0)
run_case() {
    local flags=$1
    local label=$2
    local expect_reduction=$3

    # MCA
    (
        new_env "mca_$label"
        verificarlo-c $flags --save-temps test.c -o test || {
            echo "[FAIL] MCA compilation failed ($label)"
            exit 1
        }
        if [[ $expect_reduction == 1 ]] && ! grep -qE "call .*${REDUCTION}" *.1.ll; then
            echo "[FAIL] Expected a vector reduction in the input IR ($label)"
            exit 1
        fi
        check_no_reduction_left "MCA $label"
        check_variability "MCA $label" \
            "libinterflop_mca.so --precision-binary32=20 --precision-binary64=45"
    ) || exit 1

    # PRISM
    if [[ "${BUILD_PRISM}" != "no" ]]; then
        (
            new_env "prism_$label"
            verificarlo-c --prism-backend=sr $flags --save-temps test.c -o test || {
                echo "[FAIL] PRISM compilation failed ($label)"
                exit 1
            }
            check_no_reduction_left "PRISM $label"
            check_variability "PRISM $label" "libinterflop_prism.so"
        ) || exit 1
    fi
}

# Strict FP: in-order reductions are vectorized on AArch64 only
if [[ $(arch) == "aarch64" ]]; then
    run_case "-O3" "strict" 1
else
    run_case "-O3" "strict" 0
fi

# -ffast-math: reassociable reductions are vectorized on every target
run_case "-O3 -ffast-math" "fastmath" 1

# IEEE backend must give exactly the native result for strict FP
(
    new_env ieee
    ${LLVM_BINDIR}/clang -O3 test.c -o native
    verificarlo-c -O3 test.c -o test
    expected=$(./native)
    got=$(VFC_BACKENDS_LOGGER=False VFC_BACKENDS=libinterflop_ieee.so ./test)
    if [[ "$expected" != "$got" ]]; then
        echo "[FAIL] IEEE result differs from native: $got != $expected"
        exit 1
    fi
    echo "[PASS] IEEE result matches native"
) || exit 1

echo "Test passed"
exit 0
