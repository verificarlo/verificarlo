#!/bin/bash
#set -e

Check() {
    MAX_PREC=$1
    for MODE in "MCA" ; do #FIXME test with "RR"
      for PREC in $(seq 3 10 $MAX_PREC) ; do

        echo "Checking at PRECISION $PREC MODE $MODE"
        rm -f out_mpfr out_quad
        export VFC_BACKENDS="libinterflop_mca_mpfr.so --precision $PREC --mode $MODE --seed=0"
        ./test > out_mpfr

        export VFC_BACKENDS="libinterflop_mca.so --precision $PREC --mode $MODE --seed=0"
        ./test > out_quad

	diff out_mpfr out_quad
	if [ $? -ne 0 ] ; then
          echo "error"
          exit 1
        else
          echo "ok for precision $PREC"
        fi
      done
    done
}

# Test operates at different precisions, and different operands.
# It compares that results are equivalents up to the bit.

for op in "+" "-" "*" "/" ; do
    echo "Checking $op float"
    verificarlo -D REAL=float -D SAMPLES=100 -D OPERATION="$op" -O0 test.c -o test
    Check 24

    echo "Checking $op double"
    verificarlo -D REAL=double -D SAMPLES=100 -D OPERATION="$op" -O0 test.c -o test
    Check 53
done
