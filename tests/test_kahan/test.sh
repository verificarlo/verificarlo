#!/bin/bash
set -e

export VERIFICARLO_PRECISION=53

# function_to_inst=sum_kahan
function_to_inst=sum_naive

verificarlo --function $function_to_inst -O0 kahan.c --tracer --verbose -o test

echo "z y" > output1
for z in 100; do
    for i in $(seq 1 300); do
        y=$(./test $z| grep "Kahan =" | cut -d'=' -f 2)
        echo $z $y >> output1
    done
done


verificarlo --function $function_to_inst -O3 -ffast-math  kahan.c --tracer --verbose -o test

echo "z y" > output2
for z in 100; do
    for i in $(seq 1 300); do
        y=$(./test $z| grep "Kahan =" | cut -d'=' -f 2)
        echo $z $y >> output2
    done
done
