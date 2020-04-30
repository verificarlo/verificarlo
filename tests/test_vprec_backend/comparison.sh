#!/bin/bash

print_usage() {
    echo "Usage: <float> <float> <operation> <type>"
    echo "       <float>: floating point value in hex c99 format"
    echo "       <operation>: {+,-,/,x}"
    echo "       <type>: {float,double}"
}

check_float() {
    case $1 in
	float|double) ;;
	*) print_usage; exit 1;;
    esac
}

check_op() {
    case $op in
	+|-|/|x) ;;
	*) print_usage; exit 1;;
    esac
}

print_context() {
    echo "VFC_BACKENDS=${VFC_BACKENDS}"
}

if [[ $# != 4 ]]; then
    print_usage
    exit 1
else
    x=$1
    y=$2
    op=$3
    float_type=$4    
fi

print_context

verificarlo compute_vprec_rounding.c -DREAL="${float_type}" -o compute_vprec_rounding

./compute_mpfr_rounding.py $x $y $op $float_type
./compute_vprec_rounding $x $y $op 
