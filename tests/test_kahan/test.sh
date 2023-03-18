#!/bin/bash
set -e

OPTIONS_LIST=(
    "-O0"
    "-O3 -ffast-math"
)

check_output() {
    if [[ $? != 0 ]]; then
        echo "Fail"
        exit 1
    fi
}

run() {
    BIN=$1
    ARG=$2
    ./$BIN $ARG >$BIN.log
    check_output
    grep "Kahan =" $BIN.log | cut -d'=' -f 2
}

parallel --header : "verificarlo-c -lm -DDOUBLE --function sum_kahan {OPTIONS} kahan.c -o test_{#}" ::: OPTIONS "${OPTIONS_LIST[@]}"

export VFC_BACKENDS="libinterflop_mca.so --precision-binary32 24"
export VFC_BACKENDS_LOGGER="False"

do_loop() {
    local opt=$1
    echo "z y" >output${opt}
    for z in 100; do
        for i in $(seq 1 300); do
            y=$(run test_${opt} $z)
            echo $z $y >>output${opt}
        done
    done
}

export -f check_output run do_loop

parallel --header : do_loop {opt} ::: opt $(seq ${#OPTIONS_LIST[@]})
