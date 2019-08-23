#!/bin/bash
set -e

export VFC_BACKENDS="libinterflop_mca.so"
if grep "DRAGONEGG_PATH \"\"" ../../config.h > /dev/null; then
	echo "this test is not run when using --without-dragonegg"
	exit 0
fi

# Compile newton.f90 using MCA lib instrumentation

verificarlo -c newton.f90 -o newton.o
verificarlo newton.o -o newton

# Check that two executions differ
./newton > output1
./newton > output2



if diff output1 output2; then
    echo "output should differ"
    exit 1
fi
