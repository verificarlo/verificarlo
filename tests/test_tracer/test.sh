#!/bin/bash

SAMPLES=8
ITER=30
ROOT_PATH=$PWD
LANGUAGE="C"

check_exit_code() {    
    if [ $? != 0 ]; then
       echo "${1}"
       exit 1
    fi
}

veritracer_analyze() {
    FMT=${1}
    DIR=${2}
    echo "veritracer analyze -f veritracer.dat --format=${FMT} --prefix-dir=${DIR} --output='${DIR}/veritracer' "
    veritracer analyze --filename veritracer.dat --format=${FMT} --prefix-dir=${DIR} --output="${DIR}/veritracer"
    check_exit_code "Error with veritracer analyze"
}

veritracer_plot() {
    EXP_PATH=${1}
    HASH=${2}
    OUTPUT=${3}
    veritracer plot ${EXP_PATH}/veritracer.000bt -v ${HASH} --invocation-mode  --location-info-map=locationInfo.map --no-show --output=${OUTPUT}
    check_exit_code "Error with veritracer plot"
}

veritracer_launch(){
    SAMPLES=$1
    EXP_PATH=$2
    BINARY_PATH=$3
    echo "veritracer launch --jobs=${SAMPLES} --prefix-dir=${EXP_PATH} --binary='${BINARY_PATH}' "
    veritracer launch --jobs=${SAMPLES} --prefix-dir=${EXP_PATH} --binary="${BINARY_PATH}" --force
    check_exit_code "Error with veritracer launch"
}


for FP_TYPE in "DOUBLE" "FLOAT"; do

    for FMT in "binary" "text"; do
	
	cd $ROOT_PATH
	    
	rm -rf ${LANGUAGE}
	rm locationInfo.map

	echo "${LANGUAGE} ${FP_TYPE} ${FMT}"

	if [ $LANGUAGE == "C" ]; then
	    SOURCE_FILE=muller.c
	elif [ $LANGUAGE == "FORTRAN" ]; then
	    SOURCE_FILE=muller.f90
	else
	    echo "Unknow LANGUAGE ${LANGUAGE}"
	    exit 1
	fi
	
	FUNCTION_FILE=functions_to_inst_${LANGUAGE}.txt
	echo " verificarlo -DNITER=${ITER} -D ${FP_TYPE} --functions-file=$FUNCTION_FILE --tracer --tracer-format=${FMT} --tracer-backtrace  ${SOURCE_FILE} -o muller_${FP_TYPE}_${FMT}_${LANGUAGE} -lm --verbose"
	verificarlo -DNITER=${ITER} -D ${FP_TYPE} --functions-file=${FUNCTION_FILE} --tracer --tracer-format=${FMT} --tracer-backtrace  ${SOURCE_FILE} -o muller_${FP_TYPE}_${FMT}_${LANGUAGE} -lm --verbose --tracer-debug-mode --tracer-level=temporary 
	
	check_exit_code "Error with verificarlo compilation"

	export VERIFICARLO_BACKEND=QUAD

	if [[ ${FP_TYPE} == "DOUBLE" ]]; then
	    export VERIFICARLO_PRECISION=53
	else
	    export VERIFICARLO_PRECISION=24
	fi

	EXP_PATH=${PWD}/${LANGUAGE}/${FP_TYPE}/${FMT}/${VERIFICARLO_PRECISION}bits/	   
	BINARY_PATH=./muller_${FP_TYPE}_${FMT}_${LANGUAGE}
	
	veritracer_launch ${SAMPLES} ${EXP_PATH} ${BINARY_PATH} 
	veritracer_analyze $FMT $EXP_PATH

	if [ "$LANGUAGE" == "FORTRAN" ]; then
	    HASH_MULLER_1=$(grep "ret" locationInfo.map | grep "44.0" | cut -d':' -f1)
	    HASH_MULLER_2=$(grep "ret" locationInfo.map | grep "52.0" | cut -d':' -f1)
	else
	    HASH_MULLER_1=$(grep "muller1" locationInfo.map | grep "u_k$" | cut -d':' -f1)
	    HASH_MULLER_2=$(grep "muller2" locationInfo.map | grep "x$" | cut -d':' -f1)
	fi

	echo "Hash muller1 ${HASH_MULLER_1}"
	echo "Hash muller2 ${HASH_MULLER_2}"

	OUTPUT_1="muller1_${FP_TYPE}_${FMT}_${LANGUAGE}.png"
	OUTPUT_2="muller2_${FP_TYPE}_${FMT}_${LANGUAGE}.png"

	veritracer_plot ${EXP_PATH} ${HASH_MULLER_1} ${OUTPUT_1}
	veritracer_plot ${EXP_PATH} ${HASH_MULLER_2} ${OUTPUT_2}

    done
done
