#!/bin/bash

BACKEND="MPFR QUAD BITMASK"
MODE_MCA="RR"
MODE_BITMASK="RAND"
PRECISIONS="5 23 52"
PRECISIONS=`seq 5 12 53`

rm -f *.output 

export VERIFICARLO_MCAMODE=$MODE_MCA
export VERIFICARLO_BITMASK_MODE=$MODE_BITMASK

for op in "+" "-" "*" "/"; do

    verificarlo --verbose --function=operate -D OPERATION="$op " test.c -o test
    rm -f output
    
    echo "OPERATION ${op}"
    for precision in $PRECISIONS; do
	echo "VERIFICARLO_PRECISION : ${precision}"
	export VERIFICARLO_PRECISION=$precision

	for backend in $BACKEND; do
	    echo -n "${backend} "
	    export VERIFICARLO_BACKEND=$backend
	    ./test > ${backend}.${precision}.output
	    ./compute.py ${backend}.${precision}.output >> output
	done

	echo ""
	echo "" >> output

    done

    ./compare.py output
    if [ $? != 0 ]; then
	echo "Test failed"
	exit 1
    fi

done

echo "Test passed"
