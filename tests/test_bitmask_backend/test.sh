#!/bin/bash

set -e

source ../paths.sh

SAMPLES=100
SEED=$RANDOM
echo "Seed: $SEED"

declare -A options=([FLOAT]=--precision-binary32 [DOUBLE]=--precision-binary64)
declare -A precision=([FLOAT]=24 [DOUBLE]=53)
declare -A bitmask_modes=([IB]="ib" [OB]="ob" [FULL]="full")
declare -A mca_modes=([IB]="pb" [OB]="rr" [FULL]="mca")

check_status() {
    if [[ $? != 0 ]]; then
        echo "Fail"
        exit 1
    fi
}

check_executable() {
    if [[ ! -f $1 ]]; then
        echo "Executable $1 not found"
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
if [[ $(arch) == "x86_64" ]]; then
    parallel --header : "${GCC} -DREAL={type} -D SAMPLES=$SAMPLES -O3 quadmath_stats.c -o compute_sig_{type} -lquadmath -lm" ::: type float double
else
    parallel --header : "${GCC} -DREAL={type} -D SAMPLES=$SAMPLES -O3 quadmath_stats.c -o compute_sig_{type} -lm" ::: type float double
fi
check_executable compute_sig_float
check_executable compute_sig_double

export VFC_BACKENDS_LOGGER=False

# Test operates at different precisions, and different operands.
# It compares that results are equivalents up to the bit.
parallel --header : "verificarlo-c --function=operator --verbose -D REAL={type} -D SAMPLES=$SAMPLES -O3 test.c -o test_{type} -lm" ::: type float double
check_executable test_float
check_executable test_double

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
