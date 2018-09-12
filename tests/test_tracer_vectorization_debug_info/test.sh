#!/bin/bash
set -e

export VERITRACER_LOCINFO_PATH=$PWD
export VERIFICARLO_PRECISION=53

function_to_inst=sum_kahan


check_instrumentation() {
    c=$(grep "${1}" locationInfo.map | wc -l)
    return ${c}
}


rm -f locationInfo.map

verificarlo --function $function_to_inst -O0 kahan.c --tracer --tracer-format=binary --verbose -o test --tracer-debug-mode

echo "z y" > output1
for z in 100; do
    for i in $(seq 1 300); do
        y=$(./test $z| grep "Kahan =" | cut -d'=' -f 2)
        echo $z $y >> output1
    done
done

if (( $( grep "f" locationInfo.map | wc -l ) <= 0 )); then
    echo "vectorization not found in -O0 mode"
    exit 1
fi

cat locationInfo.map
rm -f locationInfo.map

verificarlo --function $function_to_inst -O3 -ffast-math  kahan.c --tracer --tracer-format=binary --verbose -o test --tracer-debug-mode

echo "z y" > output2
for z in 100; do
    for i in $(seq 1 300); do
        y=$(./test $z| grep "Kahan =" | cut -d'=' -f 2)
        echo $z $y >> output2
    done
done

if (( $( grep "f" locationInfo.map | wc -l ) <= 0 )); then
    echo "vectorization not found in -O3 mode"
    exit 1
fi

rm -rf tmp hashf.0
veritracer launch --jobs=10 --binary="./test 10" --prefix-dir=tmp
veritracer analyze --prefix-dir=tmp
grep f.0 locationInfo.map | cut -d':' -f1 > hashf.0

if (( $( grep -f hashf.0 locationInfo.map | wc -l) <= 0 )); then
    echo "vectorization not found in -O3 mode"
    exit 1
fi
