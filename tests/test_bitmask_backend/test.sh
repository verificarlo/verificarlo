#!/bin/bash

source ../paths.sh

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

compare() {
    S_MCA=$(cat $1)
    S_BITMASK=$(cat $2)
    T=$3
    ./compare.py --s-mca $S_MCA --s-bitmask $S_BITMASK --virtual-precision=$3
    if [ $? -ne 0 ] ; then
        echo "error: |${S_MCA}-${S_BITMASK}| > 2"
        exit 1
    else
        echo "ok for precision $PREC"
        echo "|${S_MCA}-${S_BITMASK}| <= 2"
    fi
}

check() {
    MAX_PREC=$1
    START_PREC=$2
    TYPE=$3
    OPERATOR=$4
    
    for MODE in "IB" "OB" "FULL" ; do
        for PREC in $(seq $START_PREC 5 $MAX_PREC) ; do
            echo -e "\nChecking at PRECISION $PREC MODE $MODE"
            
            rm -f out_bitmask out_mca
            export VFC_BACKENDS="libinterflop_mca.so ${options[$TYPE]}=$PREC --mode=${mca_modes[$MODE]} --seed=$SEED"
            echo $VFC_BACKENDS
            ./test > out_mca
            ./compute_sig_${TYPE,,} $SAMPLES out_mca > s_mca
            
            export VFC_BACKENDS="libinterflop_bitmask.so ${options[$TYPE]}=$PREC --operator=${OPERATOR} --mode=${bitmask_modes[$MODE]} --seed=${SEED}"
            ./test > out_bitmask
            ./compute_sig_${TYPE,,} $SAMPLES out_bitmask > s_bitmask
            
            compare s_mca s_bitmask $PREC
        done
    done
}

SAMPLES=100

GCC=${GCC_PATH}
$GCC -D REAL=double -D SAMPLES=$SAMPLES -O3 quadmath_stats.c -o compute_sig_float -lquadmath
$GCC -D REAL=float -D SAMPLES=$SAMPLES -O3 quadmath_stats.c -o compute_sig_double -lquadmath

# Test operates at different precisions, and different operands.
# It compares that results are equivalents up to the bit.
for op in "+" "-" "*" "/" ; do
    echo "Checking $op float"
    verificarlo-c --function=operate --verbose -D REAL=float -D SAMPLES=$SAMPLES -D OPERATION="$op" -O0 test.c -o test
    check_status
    check 23 4 FLOAT RAND
    echo "Checking $op double"
    verificarlo-c --function=operate --verbose -D REAL=double -D SAMPLES=$SAMPLES -D OPERATION="$op" -O0 test.c -o test
    check_status
    check 52 3 DOUBLE RAND
done
