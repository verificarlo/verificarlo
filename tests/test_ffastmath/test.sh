#!/bin/bash
export VFC_BACKENDS="libinterflop_mca.so"

# Compile program with -O2 (test_1) and -O2 -ffastmath -freciprocalmath (test_2)
parallel --header : "verificarlo-c -O2 {options} test.c -o test_{#}" ::: options "" "-ffast-math -freciprocal-math"

echo "z y" >output1
for i in $(seq 1 30); do
    ./test_1 1.0 >>output1
done

echo "z y" >output2
for i in $(seq 1 30); do
    ./test_2 1.0 >>output2
done

if diff output1 output2 >/dev/null; then
    echo "output should differ"
    exit 1
fi
