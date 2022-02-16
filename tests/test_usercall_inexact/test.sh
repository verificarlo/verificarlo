#!/bin/bash

export VFC_BACKENDS_LOGGER=False
export N=30

echo $LD_LIBRARY_PATH

clean() {
    rm -f bfr.* aft.*
}

run_test() {
    BACKEND=$1
    REAL=$2
    N=$3
    RES=$4
    echo "Test ${BACKEND}"
    verificarlo-c test.c -DREAL=${REAL} -DN=${N} -o test_${REAL}
    VFC_BACKENDS="${BACKEND}" ./test_${REAL} 2>log.${REAL}
    grep "Before" log.${REAL} | grep -o '0x[0-9].[0-9a-f]*p[-+][0-9]*' >bfr.${REAL}
    grep "After" log.${REAL} | grep -o '0x[0-9].[0-9a-f]*p[-+][0-9]*' >aft.${REAL}
    diff bfr.${REAL} aft.${REAL}
    if [[ $? == ${RES} ]]; then
        echo "Test fail"
        exit 1
    fi
}

clean
for REAL in float double; do
    run_test "libinterflop_ieee.so" $REAL $N 1
    clean
    run_test "libinterflop_mca.so" $REAL $N 0
    clean
    run_test "libinterflop_mca.so -m ieee" $REAL $N 1
    clean
done
