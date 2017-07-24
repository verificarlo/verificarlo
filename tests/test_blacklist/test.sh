#!/bin/bash

verificarlo -O0 test.c -o test --functions-file functions.txt --verbose --black-list-mode > /dev/null 2> output

./test 

verificarlo -O0 test.c -o test --functions-file functions.txt --function f --verbose --black-list-mode 2> /dev/null

if [ "$?" == 0 ] ; then
    echo "Verificarlo must be stopped, --functions-file and --function should be incompatible"
    exit 1
fi

if diff output output.ref > /dev/null ; then
    echo "test passed"
    exit 0
else
    echo "ouput and output.ref should be the same"
    exit 1
fi
