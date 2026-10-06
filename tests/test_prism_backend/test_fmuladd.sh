#!/bin/bash

# Check that llvm.fmuladd (emitted by clang for a*b+c) is instrumented by the
# PRISM backend, both scalar (-O0) and vectorized (-O3).

set -e

export SAMPLES=50

if [[ "$PRISM_BACKEND" != "up-down" && "$PRISM_BACKEND" != "sr" ]]; then
    echo "Error: PRISM_BACKEND must be set to 'up-down' or 'sr'"
    exit 1
fi

export VFC_BACKENDS_LOGGER=False
export VFC_BACKENDS="libinterflop_prism.so"

mkdir -p .bin .results

for type in float double; do
    for optimization in -O0 -O3; do
        bin=.bin/test_fmuladd_${type}${optimization}
        file=.results/fmuladd.${type}${optimization}.txt

        verificarlo-c --prism-backend="${PRISM_BACKEND}" ${optimization} \
            -DREAL=${type} test_fmuladd.c -o "${bin}" --save-temps

        # The instrumented IR must not contain any native fmuladd left
        if grep -qE "call .*@llvm\.fmuladd\." test_fmuladd.*.2.ll; then
            echo "Failed! llvm.fmuladd is not instrumented (${type} ${optimization})"
            grep -E "call .*@llvm\.fmuladd\." test_fmuladd.*.2.ll | head -3
            exit 1
        fi
        rm -f test_fmuladd.*.ll test_fmuladd.*.bc

        rm -f "${file}"
        for i in $(seq 1 "$SAMPLES"); do
            "${bin}" >>"${file}"
        done

        if [[ $(sort -u "${file}" | wc -l) -le 1 ]]; then
            echo "Failed! No variability for a*b+c (${type} ${optimization})"
            sort -u "${file}"
            exit 1
        fi
    done
done

exit 0
