#!/bin/bash

verificarlo test.c -o test --function=f

export VERIFICARLO_PRECISION=40

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
