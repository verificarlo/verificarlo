#!/bin/bash

verificarlo -O0 test.c -o test --functions-file functions.txt --verbose > /dev/null 2> output

./test 

verificarlo -O0 test.c -o test --functions-file functions.txt --function f --verbose 2> /dev/null

if [ "$?" == 0 ] ; then
    echo "Verificarlo must be stopped"
    echo "--function --functions-file are incompatible"
    exit 1
fi

if ( grep "In Function: f" output && grep "In Function: g" output )  ; then
    echo "test passed"
    exit 0
else
    echo "ouput and output.ref should be the same"
    head output output.ref
    exit 1
fi
