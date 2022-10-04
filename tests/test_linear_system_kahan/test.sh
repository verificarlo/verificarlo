#!/bin/bash
set -e

export VFC_BACKENDS="libinterflop_mca.so --precision-binary64=53"

verificarlo-c --function solve -O0 linear.c -o test

echo "x y" > output
for i in $(seq 1 100); do
    ./test >> output
done

# extract x[0] samples for plotting test results
echo "X0" > x0.dat
cat output |grep e+ |cut -d' ' -f1 >> x0.dat
# To plot test results run: ./plot.R
