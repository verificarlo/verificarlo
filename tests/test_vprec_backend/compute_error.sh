#!/usr/bin/bash

TYPE=$1
RANGE=$2
USECASE=$3
RANGE_MIN=$4
RANGE_STEP=$5
PRECISION_MIN=$6
PRECISION_STEP=$7
N_SAMPLES=$8

echo "TYPE=${TYPE}"
echo "RANGE=${RANGE}"
echo "USECASE=${USECASE}"
echo "RANGE_MIN=${RANGE_MIN}"
echo "RANGE_STEP=${RANGE_STEP}"
echo "PRECISION_MIN=${PRECISION_MIN}"
echo "PRECISION_STEP=${PRECISION_STEP}"
echo "N_SAMPLES=${N_SAMPLES}"

ROOT=$PWD
SRC=$PWD/compute_vprec_rounding.c
COMPUTE_VPREC_ROUNDING=${COMPUTE_VPREC_ROUNDING}_${TYPE}
COMPUTE_MPFR_ROUNDING=$PWD/compute_mpfr_rounding.py
GENERATE_INPUT=$PWD/generate_input.py
CHECK_OUTPUT=$PWD/check_output.py

echo $COMPUTE_MPFR_ROUNDING
echo $GENERATE_INPUT

TMPDIR=$(mktemp -d -p .)
cd $TMPDIR

# Operation parameters
if [ $USECASE = "fast" ]; then
    operation_list=("+" "x")
else
    operation_list=("+" "-" "/" "x")
fi

# Modes list
if [ $USECASE = "fast" ]; then
    modes_list=("OB")
else
    modes_list=("IB" "OB" "FULL")
fi

# Range parameters
declare -A range_max
range_max["float"]=8
range_max["double"]=11
declare -A range_option
range_option["float"]=--range-binary32
range_option["double"]=--range-binary64

# Precision parameters
declare -A precision_max
precision_max["float"]=23
precision_max["double"]=52
declare -A precision_option
precision_option["float"]=--precision-binary32
precision_option["double"]=--precision-binary64

check_status() {
    if [[ $? != 0 ]]; then
        echo "Error"
        exit 1
    fi
}

compute_mpfr_rounding() {
    a=$1
    b=$2
    op=$3
    context=$4
    file=$5
    $COMPUTE_MPFR_ROUNDING $a $b $op $context >>$file
    # check_status
}

compute_vprec_rounding() {
    a=$1
    b=$2
    op=$3
    file=$4
    $COMPUTE_VPREC_ROUNDING $a $b $op >>$file
    # check_status
}

compute_op() {
    while read a b; do
        compute_mpfr_rounding $a $b $1 $2 mpfr.txt
        compute_vprec_rounding $a $b $1 vprec.txt
    done <input.txt
}

echo "TYPE: ${TYPE}"
export VERIFICARLO_VPREC_TYPE=$TYPE

echo "Range: ${RANGE}"
export VERIFICARLO_VPREC_RANGE=$RANGE
rm -f input.txt
$GENERATE_INPUT $N_SAMPLES $RANGE

for MODE in "${modes_list[@]}"; do
    echo "MODE: ${MODE}"
    export VERIFICARLO_VPREC_MODE=$MODE

    for OP in "${operation_list[@]}"; do
        echo "OP: ${OP}"
        export VERIFICARLO_OP=$OP

        for PRECISION in $(seq ${PRECISION_MIN} ${PRECISION_STEP} ${precision_max[$TYPE]}); do
            echo "Precision: ${PRECISION}"
            export VERIFICARLO_PRECISION=$PRECISION
            export VFC_BACKENDS="libinterflop_vprec.so ${precision_option[$TYPE]}=${PRECISION} ${range_option[$TYPE]}=${RANGE} --mode=${MODE}"
            echo $VFC_BACKENDS
            rm -f mpfr.txt vprec.txt
            compute_op $OP $TYPE
            $CHECK_OUTPUT 2>>log.error
        done

    done
done
