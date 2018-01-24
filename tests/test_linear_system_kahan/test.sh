#!/bin/bash
set -e

export VERIFICARLO_PRECISION=53

verificarlo --function solve -O0 linear.c -o test

echo "x y" > output
for i in $(seq 1 100); do
    ./test >> output
done
