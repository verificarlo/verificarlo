#!/bin/bash

set -e

export VFC_BACKENDS_LOGGER_SILENT_LOAD=True

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

    if grep "fptrunc" test.*.2.ll; then
        echo "[test1] cast operations not instrumented"
    else
        echo "[test1] cast operations INSTRUMENTED without --inst-cast"
        exit 1
    fi
}

test2() {
    new_env test2

    rm -f *.ll
    verificarlo-c --inst-cast -c test.c -emit-llvm --save-temps

    if grep "fptrunc" test.*.2.ll; then
        echo "[test2] comparison operations NOT instrumented with --inst-cast"
        exit 1
    else
        echo "[test2] comparison operations instrumented"
    fi
}

test3() {
    new_env test3

    rm -f *.ll
    verificarlo-c --inst-cast test.c -o test

    # Test the pb mode
    VFC_BACKENDS="libinterflop_mca.so --precision-binary64=52 --mode=pb" \
        ./test 0x1.0000010000000p-1 | sort -u >output.txt

    if [ ${PIPESTATUS[0]} != 0 ]; then
        echo "[test3] Error running the test"
        exit 1
    fi

    local lines=$(wc -l <output.txt)
    if [ $lines -eq 1 ]; then
        echo "[test3] No variability detected for PB mode"
        exit 1
    else
        echo "[test3] Variability detected for PB mode"
    fi
}

test4() {
    new_env test4

    rm -f *.ll
    verificarlo-c --inst-cast test.c -o test

    # Test the rr mode
    VFC_BACKENDS="libinterflop_mca.so --precision-binary64=23 --mode=rr" \
        ./test 0x1.fffffd000001p-1 | sort -u >output.txt

    if [ ${PIPESTATUS[0]} != 0 ]; then
        echo "Error running the test"
        exit 1
    fi

    local lines=$(wc -l <output.txt)
    if [ $lines -eq 1 ]; then
        echo "[test4] No variability detected for RR mode"
        exit 1
    else
        echo "[test4] Variability detected for RR mode"
    fi
}

export -f new_env
export -f test1 test2 test3 test4

parallel -k -j $(nproc) --halt now,fail=1 ::: test1 test2 test3 test4

if [ $? != 0 ]; then
    echo "Test failed"
    exit 1
else
    echo "Test successed"
    exit 0
fi
