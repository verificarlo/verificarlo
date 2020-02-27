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

for opt in `seq ${#OPTIONS_LIST[@]}`; do
    verificarlo-c --function sum_kahan ${OPTIONS_LIST[$opt]} kahan.c -o test
    echo "z y" > output${opt}
    for z in 100; do
	for i in $(seq 1 300); do
	    y=$(run test $z)
            echo $z $y >> output${opt}
	done
    done
done
