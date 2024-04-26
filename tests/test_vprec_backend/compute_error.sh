#!/usr/bin/bash

VERBOSE=0

# Assign command line arguments to variables
type=$1
range=$2
usecase=$3
range_min=$4
range_step=$5
precision_min=$6
precision_step=$7
n_samples=$8

if [ $VERBOSE ]; then
    # Print the variable values
    echo "Type: ${type}"
    echo "Range: ${range}"
    echo "Use Case: ${usecase}"
    echo "Range Min: ${range_min}"
    echo "Range Step: ${range_step}"
    echo "Precision Min: ${precision_min}"
    echo "Precision Step: ${precision_step}"
    echo "Number of Samples: ${n_samples}"
fi

# Define paths
root=$PWD
src="${root}/compute_vprec_rounding.c"
compute_vprec_rounding="${root}/compute_vprec_rounding_${type}"
compute_mpfr_rounding="${root}/compute_mpfr_rounding"
generate_input="${root}/generate_input.py"
check_output="${root}/check_output.py"

# Print paths
echo "Compute MPFR Rounding: ${compute_mpfr_rounding}"
echo "Generate Input: ${generate_input}"

TMPDIR=$(mktemp -d -p .)
cd $TMPDIR

# Operation parameters
if [ "$usecase" = "fast" ]; then
    operation_list=("+" "x")
else
    operation_list=("+" "-" "/" "x")
fi

# Modes list
if [ "$usecase" = "fast" ]; then
    modes_list=("OB")
else
    modes_list=("IB" "OB" "FULL")
fi

# Range parameters
declare -A range_max=(["float"]=8 ["double"]=11)
declare -A range_option=(["float"]="--range-binary32" ["double"]="--range-binary64")

# Precision parameters
declare -A precision_max=(["float"]=23 ["double"]=52)
declare -A precision_option=(["float"]="--precision-binary32" ["double"]="--precision-binary64")

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
    $compute_mpfr_rounding $a $b $op $context >>$file
    check_status
}

compute_vprec_rounding() {
    a=$1
    b=$2
    op=$3
    file=$4
    $compute_vprec_rounding $a $b $op >>$file
    check_status
}

compute_op() {
    while read a b; do
        compute_mpfr_rounding $a $b $1 $2 mpfr.txt
        compute_vprec_rounding $a $b $1 vprec.txt
    done <input.txt
    check_status
}

echo "type: ${type}"
export VERIFICARLO_VPREC_TYPE=$type

echo "range: ${range}"
export VERIFICARLO_VPREC_RANGE=$range

rm -f input.txt
$generate_input $n_samples $range
check_status

set_vfc_backends() {
    local precision=$1
    local type=$2
    local range=$3
    local mode=$4

    export VERIFICARLO_VPREC_MODE=$mode
    export VERIFICARLO_OP=$op
    export VERIFICARLO_PRECISION=$precision

    local _precision="${precision_option[$type]}=${precision}"
    local _range="${range_option[$type]}=${range}"
    local _mode="--mode=${mode}"
    export VFC_BACKENDS="libinterflop_vprec.so ${_precision} ${_range} ${_mode}"

}

for mode in "${modes_list[@]}"; do
    for op in "${operation_list[@]}"; do
        for precision in $(seq ${precision_min} ${precision_step} ${precision_max[$type]}); do
            set_vfc_backends $precision $type $range $mode
            rm -f mpfr.txt vprec.txt
            compute_op $op $type
            $check_output 2>>log.error
        done
    done
done
