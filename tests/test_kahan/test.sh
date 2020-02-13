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
    ./$BIN $ARG > log
    check_output
    grep "Kahan =" log | cut -d'=' -f 2
}

export VFC_BACKENDS="libinterflop_mca.so --precision-binary32 24"

for OPTION in "${OPTIONS_LIST[@]}"; do
    verificarlo --function sum_kahan ${OPTION} kahan.c -o test
    echo "z y" > output1
    for z in 100; do
	for i in $(seq 1 300); do
	    y=$(run test $z)
            echo $z $y >> output1
	done
    done
done
