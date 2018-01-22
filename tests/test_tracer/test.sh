#!/bin/bash

SAMPLES=16
ITER=30
FUNCTION_FILE=functions_to_inst.txt
FP_TYPE="DOUBLE" 
FMT="binary"

rm -rf ${FP_TYPE}
rm locationInfo.map

echo "${FP_TYPE} ${FMT}"
verificarlo -DNITER=${ITER} -D ${FP_TYPE} --tracer --functions-file=${FUNCTION_FILE} --tracer-fmt=${FMT} -O0 muller.c -o muller_${FP_TYPE}_${FMT}

if [ $? != 0 ]; then
    exit 1
fi

export VERIFICARLO_BACKEND=QUAD

if [[ ${FP_TYPE} == "DOUBLE" ]]; then
    export VERIFICARLO_PRECISION=53
else
    export VERIFICARLO_PRECISION=24
fi

ROOT_PATH=$PWD
EXP_PATH=${PWD}/${FP_TYPE}/${FMT}/${VERIFICARLO_PRECISION}bits/

mkdir -p $EXP_PATH

cd ${EXP_PATH}
for i in `seq ${SAMPLES}`; do
    mkdir $i
    cd $i
    ../../../../muller_${FP_TYPE}_${FMT}
    cd ..
done

veritracer_analyzer.py -f veritracer.dat -o veritracer.csv

hash_muller_1=$(grep "muller x" ../../../locationInfo.map | cut -d':' -f1)
hash_muller_2=$(grep "muller2 u_k" ../../../locationInfo.map | cut -d':' -f1)

veritracer_plot.py -f veritracer.csv_0 -v $hash_muller_1 --iteration-mode --no-transparency --no-show --output=muller_1
veritracer_plot.py -f veritracer.csv_0 -v $hash_muller_2 --iteration-mode --no-transparency --no-show --output=muller_2
