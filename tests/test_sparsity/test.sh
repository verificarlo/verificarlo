#!/bin/bash

verificarlo-c -O0 test.c -o test
export VFC_BACKENDS_SILENT_LOAD=True
export VFC_BACKENDS_LOGGER=False
export VFC_BACKENDS="libinterflop_mca.so --mode=mca --precision-binary64=24 --sparsity=100"

for sparsity in 1 2 4 10 100; do
    
    rm -f log
    export VFC_BACKENDS="libinterflop_mca.so --mode=rr --precision-binary64=1 --sparsity=${sparsity}"
    for i in `seq 1000` ; do
        ./test 0x1.fffffffffffffp2  0x1.fffffffffffffp-50 >> log 2> /dev/null
    done
    echo "Sparsity: ${sparsity} $(./parse.py log)"
    
done