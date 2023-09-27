#!/bin/bash

function run() {
    cd $1
    #/usr/bin/time -f"Elapsed time: %C %x %es"
    ./test.sh &>${1}.log
    if [[ $? != 0 ]]; then
        echo "Test failed: ${1}"
        cat ${1}.log
        exit 1
    else
        echo "Test passed: ${1}"
    fi
    cd ..
}

run test_pow2
run test_string_equal

echo "All tests passed"
exit 0
