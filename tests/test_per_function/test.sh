#!/bin/bash

export VFC_BACKENDS="libinterflop_mca.so --precision-binary64 40"
verificarlo-c test.c -o test --function=f

./test > outputf1 2> outputg1
./test > outputf2 2> outputg2

if diff outputf1 outputf2 > /dev/null ; then
    echo "f output should differ"
    exit 1
fi

if diff outputg1 outputg2 > /dev/null ; then
    echo "test passed"
    exit 0
else
    echo "g output should be the same"
    exit 1
fi
