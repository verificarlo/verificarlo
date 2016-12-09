#!/bin/bash
set -e

export VERIFICARLO_PRECISION=53

verificarlo --function sum_kahan -O0 kahan.c -o test

echo "z y" > output1
for z in 100; do
    for i in $(seq 1 300); do
        y=$(./test $z| grep "Kahan =" | cut -d'=' -f 2)
        echo $z $y >> output1
    done
done


verificarlo --function sum_kahan -O3 -ffast-math kahan.c -o test

echo "z y" > output2
for z in 100; do
    for i in $(seq 1 300); do
        y=$(./test $z| grep "Kahan =" | cut -d'=' -f 2)
        echo $z $y >> output2
    done
done
