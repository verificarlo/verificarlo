#!/bin/bash
set -e

# Test operates at different precisions, and different operands.
# It compares s': the estimated number of significant digits across the MCA samples.

../../verificarlo -O0 -lm --function operate test.c -o test

for PREC in $(seq 3 10 53) ; do

    echo "Checking at PRECISION $PREC"
    export VERIFICARLO_PRECISION=$PREC

    rm -f out_mpfr out_quad
    export VERIFICARLO_BACKEND="MPFR"
    ./test >> out_mpfr

    export VERIFICARLO_BACKEND="QUAD"
    ./test >> out_quad
    ./check.py
    if [ $? -ne 0 ] ; then
        exit $?
    fi
done
