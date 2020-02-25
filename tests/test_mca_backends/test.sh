#!/bin/bash
#set -e

SEED=$RANDOM
echo "Seed: $SEED"

declare -A options
options[float]='--precision-binary32'
options[double]='--precision-binary64'

check_status() {
    if [[ $? != 0 ]]; then
	echo "Fail"
	exit 1
    fi
}

Check() {
    TYPE=$3
    START_PREC=$2
    MAX_PREC=$1
    for MODE in "PB" "RR" "MCA" ; do
	for PREC in $(seq $START_PREC 5 $MAX_PREC) ; do
	    echo "Checking at PRECISION $PREC MODE $MODE"
	    rm -f out_mpfr out_quad
	    export VFC_BACKENDS="libinterflop_mca_mpfr.so ${options[$3]}=$PREC --mode $MODE --seed=$SEED"
	    ./test > out_mpfr
	    # MPFR returns specific nan (FFC00000) when the result is a NaN
	    # we must remove the negative sign to not break the diff comparison
	    sed -i "s\-nan\nan\g" out_mpfr

	    export VFC_BACKENDS="libinterflop_mca.so ${options[$3]}=$PREC --mode $MODE --seed=$SEED"
	    ./test > out_quad
	    sed -i "s\-nan\nan\g" out_quad

	    diff out_mpfr out_quad > log
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
    check_status
    Check 24 4 float

    echo "Checking $op double"
    verificarlo -D REAL=double -D SAMPLES=100 -D OPERATION="$op" -O0 test.c -o test
    check_status
    Check 53 3 double
done
