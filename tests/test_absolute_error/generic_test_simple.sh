#!/bin/bash
##uncomment to stop on error
#set -e
##uncomment to show all commands executed by the script
#set -x

if [[ $# != 5 ]]; then
	echo "expected 5 arguments, $# given"
	echo "usecase absErr_min absErr_step nb_tests test"
	exit 1
else
	USECASE=$1
	ABS_ERR_MIN=$2
	ABS_ERR_STEP=$3
	NB_TESTS=$4
	TEST=$5
	echo "USECASE=${USECASE}"
	echo "ABS_ERR_MIN=${ABS_ERR_MIN}"
	echo "ABS_ERR_STEP=${ABS_ERR_STEP}"
	echo "NB_TESTS=${NB_TESTS}"
	echo "TEST=${TEST}"
fi

export VERIFICARLO_BACKEND=VPREC

if [ $TEST == "additive" ]; then
	declare -A abs_error_max
	abs_error_max[0]=-126
	abs_error_max[1]=-1022
elif [ $TEST == "generative" ]; then # generative
	declare -A abs_error_max
	abs_error_max[0]=-23
	abs_error_max[1]=-52
else
	echo "Unkown test ${TEST}"
	exit 1
fi

# Operation parameters
if [ $USECASE = "fast" ]; then
	operation_list=("+" "x")
else
	operation_list=("+" "-" "x" "/")
fi

# Floating-point type list
type_list=(0 1)
declare -A type_list_names
type_list_names[0]="float"
type_list_names[1]="double"

# Modes list
if [ $USECASE = "fast" ]; then
	modes_list=("OB")
else
	modes_list=("IB" "OB" "FULL")
fi

rm -rf run_parallel

# Move out compilation to faster the test
parallel --header : "verificarlo-c -g -Wall test_${TEST}.c --function=applyOp_{type} -o test_${TEST}_{type} -lm" ::: type ${type_list_names[@]}

for TYPE in "${type_list[@]}"; do
	echo "TYPE: ${type_list_names[${TYPE}]}"
	BIN=$(realpath test_${TEST}_${TYPE})
	for MODE in "${modes_list[@]}"; do
		echo "MODE: ${MODE}"
		for OP in "${operation_list[@]}"; do
			echo "OP: ${OP}"
			for ABS_ERR in $(seq ${ABS_ERR_MIN} ${ABS_ERR_STEP} ${abs_error_max[${TYPE}]}); do
				echo "Absolute Error Threshold: ${ABS_ERR}"
				echo "./compute_error.sh ${BIN} ${MODE} ${OP} ${TYPE} ${NB_TESTS} ${ABS_ERR}" >>run_parallel
			done
		done
	done
done

parallel -j $(nproc) <run_parallel

cat >check_status.py <<HERE
import sys
import glob
paths=glob.glob('tmp.*')
ret=sum([int(open(f).readline().strip()) for f in paths]) if paths != []  else 1
print(ret)
HERE

status=$(python3 check_status.py)

if [ $status -eq 0 ]; then
	echo "Success!"
else
	echo "Failed!"
fi

rm -rf tmp.*

exit $status
