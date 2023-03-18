#!/usr/bin/env bash

BIN=$1
SPARSITY=$2

export VFC_BACKENDS_SILENT_LOAD=True
export VFC_BACKENDS_LOGGER=False
BACKENDS_OPTIONS="libinterflop_mca.so --mode=rr --precision-binary64=1 --precision-binary32=1 --sparsity=${SPARSITY}"

LOG=$(mktemp -p .)
START="$(date +%s%N)"
for i in $(seq 500); do
    export VFC_BACKENDS="${BACKENDS_OPTIONS} --seed=${i}"
    ./$BIN 0x1.fffffffffffffp2 0x1.fffffffffffffp-50 >>${LOG} 2>/dev/null
done
DURATION=$((($(date +%s%N) - ${START}) / 1000000))
perc=$(python3 parse.py ${LOG} ${SPARSITY})

rm ${LOG}

echo $? >$(mktemp -p .)
printf "Sparsity: %.3f | %8.4f%% IEEE (Time Elapsed: %4d ms)\n" $SPARSITY $perc $DURATION
