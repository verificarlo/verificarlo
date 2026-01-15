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
    export VERIFICARLO_PRECISION="$PRECISION"
    export VERIFICARLO_VPREC_RANGE="$RANGE"
    export VERIFICARLO_VPREC_MODE="$MODE"

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
    printf "%s %s\n" "$a" "$b" >"$INPUT_FILE"
    : >"$MPFR_FILE"
    : >"$VPREC_FILE"

    echo '---'

    get_info "$RANGE" "$PRECISION"

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
        echo "$@"
    fi
}

rm -f $INPUT_FILE $MPFR_FILE $VPREC_FILE

while IFS= read -r entry; do
    eval "$entry"
    run
done < <(
    awk '
        /\* (float|double):/ {
            if (match($0, /\* (float|double):/, m)) type=m[1];
            if (match($0, /MODE=([^ ]+)/, m)) mode=m[1];
            if (match($0, /RANGE=([^ ]+)/, m)) range=m[1];
            if (match($0, /PRECISION=([^ ]+)/, m)) precision=m[1];
            if (match($0, /OP=([^ ]+)/, m)) op=m[1];
        }
        /relative error too high:/ {
            if (match($0, /a=([^ ]+)/, m)) a=m[1];
            if (match($0, /b=([^ ]+)/, m)) b=m[1];
            if (type && mode && range && precision && op && a && b) {
                printf("TYPE=%s MODE=%s RANGE=%s PRECISION=%s OP=%s a=%s b=%s\n",
                       type, mode, range, precision, op, a, b);
                type=""; mode=""; range=""; precision=""; op=""; a=""; b="";
            }
        }
    ' "$ERROR_FILE"
)
