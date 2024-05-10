#!/bin/bash

# This script is used to retest the errors found in the log.error file

# log.error:
# double: MODE=OB RANGE=8 PRECISION=43 OP=+
# Relative error too high: a=-0x1.73705c6678740p-130 b=-0x1.549c60791218cp-128 mpfr=-4.975998160337267e-39 vprec=-2.938735877055719e-39 error=0.4094178127958683 (1.2883542229285385 b=2)
# mpfr=-0x1.b1787792b0400p-128 vprec=-0x1.0000000000000p-128

# Check if a filename was provided
if [ $# -eq 0 ]; then
    echo "No file provided. Please provide a file as an argument."
    echo "usage: $0 log.error"
    exit 1
else
    ERROR_FILE=$1
fi

DEBUG=False
VPREC_FILE=vprec.txt
MPFR_FILE=mpfr.txt
INPUT_FILE=input.txt

set_env() {
    export VERIFICARLO_OP="$OP"
    export VERIFICARLO_PRECISION=$PRECISION
    export VERIFICARLO_VPREC_RANGE=$RANGE
    export VERIFICARLO_VPREC_MODE=$MODE

    if [ "$TYPE" = "double" ]; then
        export VFC_BACKENDS="libinterflop_vprec.so --range-binary64=$RANGE --precision-binary64=$PRECISION --mode=$MODE"
    else
        export VFC_BACKENDS="libinterflop_vprec.so --range-binary32=$RANGE --precision-binary32=$PRECISION --mode=$MODE"
    fi

    export VERIFICARLO_VPREC_TYPE=$TYPE

    export compute_mpfr_rounding="./compute_mpfr_rounding"
    export compute_vprec_rounding="./compute_vprec_rounding_${TYPE}"
    export compute_cpfloat_rounding="./compute_cpfloat_rounding"

    echo "VFC_BACKENDS=$VFC_BACKENDS"
}

get_info() {
    range=$1
    precision=$2
    python3 print_info_fp.py $range $precision
}

mpfr_rounding() {
    a=$1
    b=$2
    op=$3
    context=$4
    debug "mpfr $a $b $op $context"
    $compute_mpfr_rounding $a $b $op $context
}

vprec_rounding() {
    a=$1
    b=$2
    op=$3
    debug "vprec $a $b $op"
    $compute_vprec_rounding $a $b $op
}

run() {
    set_env

    echo '---'

    get_info $RANGE $PRECISION

    export VFC_BACKENDS_SILENT_LOAD=True
    export VFC_BACKENDS_LOGGER=True
    export VFC_BACKENDS_LOGGER_LEVEL=debug
    export VFC_BACKENDS_COLORED_LOGGER=True

    echo "MPFR ROUNDING"
    VERBOSE_MODE='' mpfr_rounding $a $b "$VERIFICARLO_OP" $TYPE >>$MPFR_FILE
    echo
    echo "VPREC ROUNDING"
    vprec_rounding $a $b "$VERIFICARLO_OP" >>$VPREC_FILE
    echo

    export VERIFICARLO_EXIT_AT_ERROR=False
    python3 check_output.py

}

debug() {
    if [ "$DEBUG" = "True" ]; then
        echo $@
    fi
}

rm -f $INPUT_FILE $MPFR_FILE $VPREC_FILE

nb_lines=$(wc -l "$ERROR_FILE" | cut -d' ' -f1)

# Iterate over the lines of the log.error file 3 by 3
for ((i = 3; i <= $nb_lines; i = i + 3)); do

    # Set TYPE to double if the line contains "double"
    if head -n$i "$ERROR_FILE" | tail -n3 | grep -q "double" "$1"; then
        TYPE="double"
    else
        TYPE="float"
    fi

    # Iterate over the array and set the environment variables
    variables="$(head -n$i "$ERROR_FILE" | tail -n3 | grep -o '[A-Za-z0-9_]*=[^[:space:]]*')"

    for line in $variables; do
        debug "line:" $line
        IFS='=' read -r -a pair <<<"$line"
        export ${pair[0]%:}=${pair[1]}
        debug "Set variable ${pair[0]} to ${pair[1]}"
    done

    echo "${a} ${b}" >>$INPUT_FILE

    run
done
