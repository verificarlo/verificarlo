#!/bin/bash
set -e

export VERIFICARLO_PRECISION=24
export SAMPLES=1000

../../verificarlo -O0 --function main test.c -o test

export VERIFICARLO_BACKEND="MPFR"
echo "c" > out_mpfr
for z in $(seq 1 $SAMPLES); do
    ./test >> out_mpfr
done

export VERIFICARLO_BACKEND="QUAD"
echo "c" > out_quad
for z in $(seq 1 $SAMPLES); do
    ./test >> out_quad
done

exit 0
