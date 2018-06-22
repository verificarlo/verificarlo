#!/bin/bash

verificarlo -O0 test.c -o test --functions-file functions.txt --verbose --black-list-mode > /dev/null 2> output

./test 

verificarlo -O0 test.c -o test --functions-file functions.txt --function f --verbose --black-list-mode 2> /dev/null

if [ "$?" == 0 ] ; then
    echo "Verificarlo must be stopped, --functions-file and --function should be incompatible"
    exit 1
fi
if  (( $(grep -c "In Function: f" output) && $(grep -c "In Function: g" output) ))   ; then
    echo "test passed"
    exit 0
else
    echo "Error while instrumenting"
    cat output
    exit 1
fi
