#!/bin/bash

source ../paths.sh

SAMPLES=100
SEED=$RANDOM
echo "Seed: $SEED"

declare -A options
options[FLOAT]=--precision-binary32
options[DOUBLE]=--precision-binary64

declare -A precision
precision[FLOAT]=24
precision[DOUBLE]=53

declare -A bitmask_modes
declare -A mca_modes
bitmask_modes=([IB]="ib" [OB]="ob" [FULL]="full")
mca_modes=([IB]="pb" [OB]="rr" [FULL]="mca")

check_status() {
    if [[ $? != 0 ]]; then
        echo "Fail"
        exit 1
    fi
}

check() {
    MAX_PREC=$1
    START_PREC=$2
    TYPE=$3
    OPERATOR=$4
    SEED=$5
    OP=$6

    for MODE in "IB" "OB" "FULL"; do
        for PREC in $(seq $START_PREC 5 $MAX_PREC); do
            echo "./compute_error.sh ${TYPE} ${OPERATOR} ${MODE} ${PREC} ${SEED} ${SAMPLES} ${OP}" >>run_parallel
        done
    done
}

GCC=${GCC_PATH}
$GCC -D REAL=double -D SAMPLES=$SAMPLES -O3 quadmath_stats.c -o compute_sig_float -lquadmath
$GCC -D REAL=float -D SAMPLES=$SAMPLES -O3 quadmath_stats.c -o compute_sig_double -lquadmath

export VFC_BACKENDS_LOGGER=False

# Test operates at different precisions, and different operands.
# It compares that results are equivalents up to the bit.
verificarlo-c --function=operator --verbose -D REAL=float -D SAMPLES=$SAMPLES -O0 test.c -o test_float
verificarlo-c --function=operator --verbose -D REAL=double -D SAMPLES=$SAMPLES -O0 test.c -o test_double

rm -f run_parallel

for op in "+" "-" "x" "/"; do
    check 23 4 FLOAT RAND ${SEED} ${op}
    check 52 3 DOUBLE RAND ${SEED} ${op}
done

parallel -j $(nproc) <run_parallel

cat >check_status.py <<HERE
import sys
import glob
paths=glob.glob('tmp.*/output.txt')
ret=sum([int(open(f).readline().strip()) for f in paths]) if paths != []  else 1
print(ret)
HERE

status=$(python3 check_status.py)

if [ $status -eq 0 ]; then
    echo "Success!"
else
    echo "Failed!"
fi

rm -rf tmp.*

exit $status
