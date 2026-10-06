#!/bin/bash
# Test that vector operations wider than the target vector registers are
# passed to the MCA wrappers following the target calling convention: with
# the IEEE backend, comparisons (--inst-fcmp) and arithmetic must give exactly
# the native results, for every ISA target the host can run.

source ../paths.sh

case $(arch) in
aarch64)
    targets=("-march=armv8-a")
    grep -qw sve /proc/cpuinfo && targets+=("-march=armv8-a+sve")
    grep -qw sve2 /proc/cpuinfo && targets+=("-march=armv9-a+sve2")
    ;;
x86_64)
    targets=("-march=x86-64")
    grep -qw avx2 /proc/cpuinfo && targets+=("-mavx2")
    grep -qw avx512f /proc/cpuinfo && targets+=("-mavx512f")
    ;;
*)
    echo "Unsupported architecture: $(arch), skipping"
    exit 77
    ;;
esac

${LLVM_BINDIR}/clang -O1 test.c -o native || exit 1
expected=$(./native)

status=0
for target in "${targets[@]}"; do
    rm -f test
    if ! verificarlo-c -O1 $target --inst-fcmp test.c -o test; then
        echo "[FAIL] Compilation failed ($target)"
        status=1
        continue
    fi
    got=$(VFC_BACKENDS_LOGGER=False VFC_BACKENDS=libinterflop_ieee.so ./test)
    if [[ "$got" != "$expected" ]]; then
        echo "[FAIL] Results differ from native ($target)"
        echo "  native: $expected"
        echo "  got:    $got"
        status=1
    else
        echo "[PASS] $target"
    fi
done

rm -f native test
exit $status
