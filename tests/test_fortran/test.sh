#!/bin/bash
set -e

# Compile newton.f90 using MCA lib instrumentation

../../verificarlo -c newton.f90 -o newton.o
../../verificarlo newton.o -o newton

# Check that two executions differ
./newton > output1
./newton > output2

diff output1 output2 > /dev/null

if [ "x$?" == "x0" ]; then
    echo "output should differ"
    exit 1
fi
