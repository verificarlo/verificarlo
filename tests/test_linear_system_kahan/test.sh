#!/bin/bash
set -e

export VFC_BACKENDS="libinterflop_mca.so --precision-binary64=53"

verificarlo-c --function solve -O0 linear.c -o test

echo "x y" > output
for i in $(seq 1 100); do
    ./test >> output
done
