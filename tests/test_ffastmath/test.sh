#!/bin/bash
export VFC_BACKENDS="libinterflop_mca.so"

# Compile program with -O2
verificarlo -O2 test.c -o test

echo "z y" > output1
for i in $(seq 1 30); do
    ./test 1.0 >> output1
done


# Compile program with -O2 -ffastmath -freciprocalmath
verificarlo -O2 -ffast-math -freciprocal-math test.c -o test

echo "z y" > output2
for i in $(seq 1 30); do
    ./test 1.0 >> output2
done

if diff output1 output2 > /dev/null ; then
    echo "output should differ"
    exit 1
fi
