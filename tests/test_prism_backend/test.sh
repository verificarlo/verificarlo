#!/bin/bash

function run() {
    local mode=$1
    local msg=$2
    local cmd=$3
    local static_dispatch=${4:-0}
    local march_native=${5:-0}
    local xfail=${6:-0}

    echo $msg
    PRISM_BACKEND=$mode STATIC_DISPATCH=${static_dispatch} MARCH_NATIVE=${march_native} ./$cmd

    if [[ $? != 0 ]]; then
        if [[ $xfail == 1 ]]; then
            echo "XFAIL"
        else
            echo "FAIL"
            exit 1
        fi
    else
        echo "PASS"
    fi
    ./clean.sh
}

DYNAMIC=0
STATIC=1

BASELINE=0
NATIVE=1

PASS=0
XFAIL=1

for mode in up-down sr; do

    # SCALAR TESTS
    run $mode "Test scalar dynamic dispatch" test_scalar.sh $DYNAMIC $BASELINE
    run $mode "Test scalar static dispatch" test_scalar.sh $STATIC $BASELINE $XFAIL
    run $mode "Test scalar dynamic dispatch -march=native" test_scalar.sh $DYNAMIC $NATIVE
    run $mode "Test scalar static dispatch -march=native" test_scalar.sh $STATIC $NATIVE

    # VECTOR TESTS
    run $mode "Test vector dynamic dispatch" test_vector.sh $DYNAMIC $BASELINE
    # Supposed to fail since target binary and prism library are not compiled with the same target flags
    run $mode "Test vector static dispatch" test_vector.sh $STATIC $BASELINE $XFAIL
    run $mode "Test vector dynamic dispatch -march=native" test_vector.sh $DYNAMIC $NATIVE
    run $mode "Test vector static dispatch -march=native" test_vector.sh $STATIC $NATIVE

done

exit 0
