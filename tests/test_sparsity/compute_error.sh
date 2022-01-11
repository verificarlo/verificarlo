#!/usr/bin/env bash

BIN=$1
SPARSITY=$2

TMPDIR=$(mktemp -d -p .)
cd $TMPDIR

export VFC_BACKENDS_SILENT_LOAD=True
export VFC_BACKENDS_LOGGER=False

START="$(date +%s%N)"
for i in $(seq 500); do
    export VFC_BACKENDS="libinterflop_mca.so --mode=rr --precision-binary64=1 --precision-binary32=1 --sparsity=${SPARSITY} --seed=${i}"
    $BIN 0x1.fffffffffffffp2 0x1.fffffffffffffp-50 >>log 2>/dev/null
done
DURATION=$((($(date +%s%N) - ${START}) / 1000000))
perc=$(../parse.py log ${SPARSITY})
echo $? >output.log
echo "  Sparsity: ${SPARSITY} | ${perc}% IEEE (Time Elapsed: $DURATION ms)"
