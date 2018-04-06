#!/bin/bash

SAMPLES=16
ITER=30
FP_TYPE="DOUBLE" 
FMT="binary"
ROOT_PATH=$PWD

for language in c  fortran; do 

    cd $ROOT_PATH
    
    rm -rf ${language}
    rm locationInfo.map

    echo "${language} ${FP_TYPE} ${FMT}"

    if [ $language == "c" ]; then
	SOURCE_FILE=muller.c
    elif [ $language == "fortran" ]; then
	SOURCE_FILE=muller.f90
    else
	echo "Unknow language ${language}"
	exit 1
    fi
    
    FUNCTION_FILE=functions_to_inst_${language}.txt
    echo " verificarlo -DNITER=${ITER} -D ${FP_TYPE} --functions-file=$FUNCTION_FILE --tracer --tracer-fmt=${FMT} --tracer-backtrace  ${SOURCE_FILE} -o muller_${FP_TYPE}_${FMT}_${language} -lm --verbose"
    verificarlo -DNITER=${ITER} -D ${FP_TYPE} --functions-file=$FUNCTION_FILE --tracer --tracer-fmt=${FMT} --tracer-backtrace  ${SOURCE_FILE} -o muller_${FP_TYPE}_${FMT}_${language} -lm --verbose 

    # sort -nu locationInfo.map
    
    if [ $? != 0 ]; then
	cat log.${language}.err
	exit 1
    fi

    export VERIFICARLO_BACKEND=QUAD

    if [[ ${FP_TYPE} == "DOUBLE" ]]; then
	export VERIFICARLO_PRECISION=53
    else
	export VERIFICARLO_PRECISION=24
    fi

    EXP_PATH=${PWD}/${language}/${FP_TYPE}/${FMT}/${VERIFICARLO_PRECISION}bits/
    mkdir -p $EXP_PATH
    cd ${EXP_PATH}
    for i in `seq ${SAMPLES}`; do
	mkdir $i
	cd $i
	../../../../../muller_${FP_TYPE}_${FMT}_${language}
	cd ..
    done

    veritracer_analyzer.py -f veritracer.dat -o veritracer.csv

    hash_muller_1=$(grep "muller u_k1" ../../../../locationInfo.map | cut -d':' -f1)
    hash_muller_2=$(grep "muller2 x" ../../../../locationInfo.map | cut -d':' -f1)

    veritracer_plot.py -f veritracer.csv_0 -v $hash_muller_1 --iteration-mode --no-transparency --location-info-map=../../../../locationInfo.map --no-show --output=muller_1
    veritracer_plot.py -f veritracer.csv_0 -v $hash_muller_2 --iteration-mode --no-transparency --location-info-map=../../../../locationInfo.map --no-show --output=muller_2

done
