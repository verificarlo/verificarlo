#!/bin/bash

set -e

new_env() {
    DIR=.$1
    rm -rf $DIR
    mkdir $DIR
    cp -r *.c $DIR
    cd $DIR
}

test1() {
    new_env test1

    rm -f *.ll
    verificarlo-c -c test.c -emit-llvm --save-temps

    if grep "fcmp ogt" test.*.2.ll || grep "fcmp ole" test.*.2.ll; then
        echo "[TEST 1 | PASS] comparison operations not instrumented"
    else
        echo "[TEST 1 | FAIL] comparison operations INSTRUMENTED without --inst-fcmp"
        exit 1
    fi
}

test2() {
    new_env test2

    rm -f *.ll
    verificarlo-c --inst-fcmp -c test.c -emit-llvm --save-temps

    if grep "fcmp ogt" test.*.2.ll || grep "fcmp ole" test.*.2.ll; then
        echo "[TEST 2 | FAIL] comparison operations NOT instrumented with --inst-fcmp"
        exit 1
    else
        echo "[TEST 2 | PASS] comparison operations instrumented"
    fi

    # Only test vector comparisons in x86_64
    if [[ $(arch) == "x86_64" ]]; then
        if grep "_4xdoublecmp" test.*.2.ll; then
            echo "[TEST 2 | PASS] vector comparison instrumented"
        else
            echo "[TEST 2 | FAIL] vector comparison NOT instrumented with --inst-fcmp"
            exit 1
        fi
    fi
}

test3() {
    new_env test3

    # Test correct interposition for scalar and vector cases
    verificarlo-c --inst-fcmp run.c -o run
    VFC_BACKENDS="libinterflop_ieee.so --debug" ./run | sort -n 2>scalar.log

    verificarlo-c --inst-fcmp -O2 run.c -o run
    VFC_BACKENDS="libinterflop_ieee.so --debug" ./run | sort -n 2>vector.log

    if diff scalar.log vector.log; then
        echo "[TEST 3 | PASS] Test succeeded"
        exit 0
    else
        echo "[TEST 3 | FAIL] Test failed"
        exit 1
    fi
}

export -f new_env
export -f test1 test2 test3

parallel -k -j $(nproc) ::: test1 test2 test3
