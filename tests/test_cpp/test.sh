#!/bin/bash

verificarlo-c++ test.cpp -o test --verbose

export VFC_BACKENDS="libinterflop_ieee.so --debug"
./test 2> output

diff ref output

if [ $? == 0 ]; then
    echo "pass"
    exit 0
else
    echo "fail"
    exit 1
fi
   
   
