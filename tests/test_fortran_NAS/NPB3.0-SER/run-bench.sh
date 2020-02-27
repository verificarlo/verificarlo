#!/bin/bash

BASEDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
CLASS=S
export VFC_BACKENDS="libinterflop_ieee.so"

check_success() {
    if [[ $? != 0 ]]; then
	echo "Benchmark failed"
	exit 1
    fi
}

make clean

for bench in CG; do
    echo "Benchmarking $bench"
    sbench=$( echo $bench | tr '[A-Z]' '[a-z]' )
    cd $BASEDIR/$bench
    make CLASS=${CLASS}
    check_success
    ../bin/${sbench}.${CLASS}
    check_success
done
