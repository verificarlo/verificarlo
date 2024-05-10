#!/bin/bash

check_status() {
    if [ $? -ne 0 ]; then
        echo "Test fail"
        exit 1
    fi
}

clean() {
    rm -f *.log
}

clean
verificarlo-c test.c -o test
check_status

VFC_BACKENDS="libinterflop_mca.so --precision-binary32 23 --seed=1234" ./test 0 0 >23.log 2>/dev/null
VFC_BACKENDS="libinterflop_mca.so --precision-binary32 10 --seed=1234" ./test 0 0 >10.log 2>/dev/null
VFC_BACKENDS="libinterflop_mca.so --precision-binary32 23 --seed=1234" ./test 1 0 >change.log 2>/dev/null

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

VFC_BACKENDS="libinterflop_vprec.so --precision-binary32 23" ./test 0 0 >23.log 2>/dev/null
VFC_BACKENDS="libinterflop_vprec.so --precision-binary32 10" ./test 0 0 >10.log 2>/dev/null
VFC_BACKENDS="libinterflop_vprec.so --precision-binary32 23" ./test 1 0 >change.log 2>/dev/null

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

VFC_BACKENDS="libinterflop_vprec.so --precision-binary32 23 --range-binary32=8" ./test 0 0 >r8.log 2>/dev/null
VFC_BACKENDS="libinterflop_vprec.so --precision-binary32 23 --range-binary32=2" ./test 0 0 >r2.log 2>/dev/null
VFC_BACKENDS="libinterflop_vprec.so --precision-binary32 23 --range-binary32=8" ./test 0 1 >change.log 2>/dev/null

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
