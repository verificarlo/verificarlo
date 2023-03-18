#!/bin/bash
##uncomment to stop on error
#set -e
##uncomment to show all command executed by the script
#set -x

export VFC_BACKENDS_LOGGER=False

if [[ $# != 6 ]]; then
	echo "expected 5 arguments, $# given"
	echo "usecase range_min range_step precision_min precision_step n_samples"
	exit 1
else
	USECASE=$1
	RANGE_MIN=$2
	RANGE_STEP=$3
	PRECISION_MIN=$4
	PRECISION_STEP=$5
	N_SAMPLES=$6
	echo "USECASE=${USECASE}"
	echo "RANGE_MIN=${RANGE_MIN}"
	echo "RANGE_STEP=${RANGE_STEP}"
	echo "PRECISION_MIN=${PRECISION_MIN}"
	echo "PRECISION_STEP=${PRECISION_STEP}"
	echo "N_SAMPLES=${N_SAMPLES}"
fi

export VERIFICARLO_BACKEND=VPREC

# Operation parameters
if [ $USECASE = fast ]; then
	operation_list=("+" "x")
else
	operation_list=("+" "-" "/" "x")
fi

# Floating type list
float_type_list=("float" "double")

# Modes list
if [ $USECASE = "fast" ]; then
	modes_list=("OB")
else
	modes_list=("IB" "OB" "FULL")
fi

# Range parameters
declare -A range_max
range_max["float"]=8
range_max["double"]=11
declare -A range_option
range_option["float"]=--range-binary32
range_option["double"]=--range-binary64

# Precision parameters
declare -A precision_max
precision_max["float"]=23
precision_max["double"]=52
declare -A precision_option
precision_option["float"]=--precision-binary32
precision_option["double"]=--precision-binary64

rm -f log.error
rm -f run_parallel.sh

parallel --header : "verificarlo-c compute_vprec_rounding.c -DREAL={type} -o compute_vprec_rounding_{type} --verbose" ::: type float double

export COMPUTE_VPREC_ROUNDING=$(realpath compute_vprec_rounding)

for TYPE in "${float_type_list[@]}"; do
	for RANGE in $(seq ${RANGE_MIN} ${RANGE_STEP} ${range_max[$TYPE]}); do
		echo "./compute_error.sh $TYPE $RANGE $USECASE $RANGE_MIN $RANGE_STEP $PRECISION_MIN $PRECISION_STEP $N_SAMPLES" >>run_parallel
	done
done

parallel -j $(nproc) <run_parallel

cat tmp.*/log.error >log.error

rm -rf tmp.*

cat log.error

if [ -s "log.error" ]; then
	echo "Test failed"
	exit 1
else
	echo "Test suceed"
	exit 0
fi
