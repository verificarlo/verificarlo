#!/bin/bash

export VFC_BACKENDS_LOGGER=False
export N=30

check_status() {
    if [[ $? != 0 ]]; then
        echo "Test fail"
        exit 1
    fi
}

clean() {
    rm -f bfr.* aft.*
}

run_test() {
    BACKEND=$1
    REAL=$2
    P=$3
    RES=$4
    echo "Test ${BACKEND}"
    VFC_BACKENDS="${BACKEND}" ./test_${REAL} ${P} 2>log.${REAL}
    grep "Before" log.${REAL} | grep -o '0x[0-9].[0-9a-f]*p[-+][0-9]*' >bfr.${REAL}
    grep "After" log.${REAL} | grep -o '0x[0-9].[0-9a-f]*p[-+][0-9]*' >aft.${REAL}
    diff bfr.${REAL} aft.${REAL}
    if [[ $? == ${RES} ]]; then
        echo "Test fail"
        exit 1
    fi
}

# Move out compilation to faster test
parallel --header : "make --silent type={type} N=${N}" ::: type float double
check_status

clean
for REAL in float double; do
    # IEEE backend, no noise must be introduced
    run_test "libinterflop_ieee.so" $REAL 0 1
    clean
    # MCA backend, inexact(virtual_precision=0) equivalent to use the current virtual precision
    # By default 53 and 24 bits so it is equivalent to not perturb the value (since RR preserves FPs representable on p bits)
    run_test "libinterflop_mca.so" $REAL 0 1
    clean
    # MCA backend, inexact(virtual_precision=-1) equivalent to use the current virtual precision - 1
    # Equivalent to use 52 and 23 bits of virtual precision. Results must be slighty different
    run_test "libinterflop_mca.so" $REAL -1 0
    clean
    # MCA backend, inexact(virtual_precision=1) equivalent to use a virtual precision of 1
    # The minimal virtual precision must introduce a large noise.
    run_test "libinterflop_mca.so" $REAL 1 0
    clean
    # MCA backend within IEEE mode must not introduce noise at all.
    run_test "libinterflop_mca.so -m ieee" $REAL -1 1
    clean
done
