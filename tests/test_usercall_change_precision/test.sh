#!/bin/bash

clean() {
    rm -f *.log
}

clean
verificarlo-c test.c -o test

VFC_BACKENDS="libinterflop_mca.so --precision-binary32 23 --seed=1234" ./test 0 0 2> 23.log > /dev/null
VFC_BACKENDS="libinterflop_mca.so --precision-binary32 10 --seed=1234" ./test 0 0 2> 10.log > /dev/null
VFC_BACKENDS="libinterflop_mca.so --precision-binary32 23 --seed=1234" ./test 1 0 2> change.log > /dev/null

DIFF=$(diff change.log 23.log)
if [[ $DIFF == "" ]]; then
        echo "Test fail for mca"
        exit 1
    fi

DIFF=$(diff change.log 10.log)
if [[ $DIFF != "" ]]; then
        echo "Test fail for mca"
        exit 1
    fi

VFC_BACKENDS="libinterflop_vprec.so --precision-binary32 23" ./test 0 0 2> 23.log > /dev/null
VFC_BACKENDS="libinterflop_vprec.so --precision-binary32 10" ./test 0 0 2> 10.log > /dev/null
VFC_BACKENDS="libinterflop_vprec.so --precision-binary32 23" ./test 1 0 2> change.log > /dev/null

DIFF=$(diff change.log 23.log)
if [[ $DIFF == "" ]]; then
        echo "Test fail for vprec precision"
        exit 1
    fi

DIFF=$(diff change.log 10.log)
if [[ $DIFF != "" ]]; then
        echo "Test fail for vprec precision"
        exit 1
    fi

VFC_BACKENDS="libinterflop_vprec.so --precision-binary32 23 --range-binary32=8" ./test 0 0 2> r8.log > /dev/null
VFC_BACKENDS="libinterflop_vprec.so --precision-binary32 23 --range-binary32=2" ./test 0 0 2> r2.log > /dev/null
VFC_BACKENDS="libinterflop_vprec.so --precision-binary32 23 --range-binary32=8" ./test 0 1 2> change.log > /dev/null

DIFF=$(diff change.log r8.log)
if [[ $DIFF == "" ]]; then
        echo "Test fail for vprec range"
        exit 1
    fi

DIFF=$(diff change.log r2.log)
if [[ $DIFF != "" ]]; then
        echo "Test fail for vprec range"
        exit 1
    fi