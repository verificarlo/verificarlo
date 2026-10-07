#!/bin/bash
# Test the instrumentation of floating-point operations on scalable vectors
# (SVE on AArch64): loops vectorized with scalable vectors, and predicated SVE
# intrinsics. Both passes replace each such operation by a loop over its
# lanes calling the scalar instrumentation function on the active lanes.
#
# For each SVE vector length the host can run (natively, and 256, 512 and
# 2048 bits with QEMU user mode if available):
#   MCA pass:
#     1. IEEE backend: same results as native, bit for bit
#     2. VPREC backend: each kernel matches its scalar counterpart (test.c),
#        and differs from native: every lane is instrumented
#     3. the backend is called once per active lane of a predicated operation
#   PRISM pass (SR):
#     4. exact inputs: same results as native
#     5. inexact inputs: results vary across runs, inactive lanes unchanged

source ../paths.sh

if [[ $(arch) != "aarch64" ]]; then
    echo "Not an AArch64 host, skipping SVE scalable vectors test"
    exit 77
fi

FLAGS="-O2 -march=armv8-a+sve"
VPREC="libinterflop_vprec.so --precision-binary64=20 --precision-binary32=10 --mode=full"
export VFC_BACKENDS_LOGGER=False

# Commands running a binary with SVE, one per vector length
runners=()
if grep -qw sve /proc/cpuinfo; then
    runners+=("")
fi
QEMU=${QEMU:-$(command -v qemu-aarch64-static || command -v qemu-aarch64)}
if [[ -n "$QEMU" ]]; then
    for bits in 256 512 2048; do
        runners+=("$QEMU -cpu max,sve-default-vector-length=$((bits / 8))")
    done
else
    echo "QEMU user mode not found, only testing the native vector length"
fi

fail() {
    echo "[FAIL] $*"
    exit 1
}

vector_length() {
    if [[ -z "$1" ]]; then
        echo "native"
    else
        echo "$((${1##*=} * 8)) bits"
    fi
}

# No floating-point arithmetic on scalable vectors may be left native. The
# PRISM pass does not instrument comparisons.
check_no_native_scalable() {
    local ops="fadd|fsub|fmul|fdiv"
    [[ "$1" == "fcmp" ]] && ops="$ops|fcmp [a-z]+"
    local sve="f(add|sub|subr|mul|div|divr|mla|mls|nmla|nmls|mad|msb|nmad|nmsb)"
    if ! grep -q "<vscale x" *.1.ll; then
        fail "Expected scalable vectors in the input IR"
    fi
    if grep -E "= ($ops) [a-z ]*<vscale x|call .*@llvm\.(fma|fmuladd|vector\.reduce\.f(add|mul))\.nxv|call .*@llvm\.aarch64\.sve\.$sve(\.u)?\.nxv" *.2.ll; then
        fail "Native scalable vector operations left in the IR"
    fi
    echo "[PASS] No native scalable vector operations"
}

# All kernels match their scalar counterpart, inactive lanes are unchanged
check_all_ok() {
    if [[ $(wc -l <"$1") -ne 52 ]] || grep -v "active=OK inactive=OK" "$1"; then
        fail "$2"
    fi
}

hashes() {
    awk '{print $1, $2}' "$1"
}

# Kernels with the same results in both files
same_kernels() {
    join <(hashes "$1") <(hashes "$2") | awk '$2 == $3 {print $1}' | xargs
}

${LLVM_BINDIR}/clang $FLAGS test.c -o .native || fail "Native compilation failed"

test_mca() {
    rm -rf .mca && mkdir .mca && cd .mca
    verificarlo-c $FLAGS --inst-fma --inst-fcmp --save-temps ../test.c \
        -o test || fail "MCA compilation failed"
    check_no_native_scalable fcmp
    # VPREC does not instrument comparisons
    verificarlo-c $FLAGS --inst-fma ../test.c -o test_vprec ||
        fail "MCA compilation without --inst-fcmp failed"

    for runner in "${runners[@]}"; do
        vl=$(vector_length "$runner")
        $runner ../.native >native.out
        VFC_BACKENDS=libinterflop_ieee.so $runner ./test >ieee.out
        check_all_ok ieee.out "IEEE: kernels differ from scalar code ($vl)"
        cmp -s native.out ieee.out || fail "IEEE: results differ from native ($vl)"
        echo "[PASS] IEEE results are native ($vl)"

        VFC_BACKENDS="$VPREC" $runner ./test_vprec >vprec.out
        check_all_ok vprec.out "VPREC: kernels differ from scalar code ($vl)"
        same=$(same_kernels native.out vprec.out)
        [[ "$same" == "cmp_f64 cmp_f32" ]] ||
            fail "VPREC: kernels not instrumented ($vl): $same"
        echo "[PASS] VPREC results match scalar code and are instrumented ($vl)"

        VFC_BACKENDS="libinterflop_ieee.so --count-op" $runner ./test count \
            >count.out 2>&1
        active=$(sed -n 's/^active lanes: //p' count.out)
        adds=$(sed -n 's/^\s*add=//p' count.out)
        [[ -n "$active" && "$adds" == "$active" ]] ||
            fail "Backend called $adds times for $active active lanes ($vl)"
        echo "[PASS] Backend called on the $active active lanes only ($vl)"
    done
}

test_prism() {
    if [[ "${BUILD_PRISM}" == "no" ]]; then
        echo "[SKIP] PRISM not built"
        return
    fi
    rm -rf .prism && mkdir .prism && cd .prism
    verificarlo-c --prism-backend=sr $FLAGS --save-temps ../test.c -o test ||
        fail "PRISM compilation failed"
    check_no_native_scalable

    export VFC_BACKENDS=libinterflop_prism.so
    for runner in "${runners[@]}"; do
        vl=$(vector_length "$runner")
        $runner ../.native exact >native.out
        $runner ./test exact >exact.out
        cmp -s native.out exact.out || fail "PRISM: exact results differ from native ($vl)"
        echo "[PASS] PRISM exact results are native ($vl)"

        $runner ./test >run1.out
        $runner ./test >run2.out
        grep -v "inactive=OK" run1.out && fail "PRISM: inactive lanes modified ($vl)"
        same=$(same_kernels run1.out run2.out)
        [[ "$same" == "cmp_f64 cmp_f32" ]] ||
            fail "PRISM: no variability ($vl): $same"
        echo "[PASS] PRISM results vary, inactive lanes unchanged ($vl)"
    done
}

# Reductions allowed to be reordered (fast-math) are instrumented as well
test_fast_math() {
    rm -rf .fast && mkdir .fast && cd .fast
    verificarlo-c $FLAGS -ffast-math --inst-fma --save-temps ../test.c -o test ||
        fail "Compilation with -ffast-math failed"
    check_no_native_scalable
}

(test_mca) || exit 1
(test_prism) || exit 1
(test_fast_math) || exit 1

if [[ ${#runners[@]} -eq 0 ]]; then
    echo "No SVE CPU and no QEMU: binaries were not run"
    exit 77
fi

echo "Test passed"
exit 0
