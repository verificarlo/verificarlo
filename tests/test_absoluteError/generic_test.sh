#!/bin/bash
##uncomment to stop on error
#set -e
##uncomment to show all commands executed by the script
#set -x

if [[ $# != 8 ]]; then
    echo "expected 8 arguments, $# given"
    echo "usecase range_min range_step precision_min precision_step absErr_min absErr_step nb_tests"
    exit 1
else
    USECASE=$1
    RANGE_MIN=$2
    RANGE_STEP=$3
    PRECISION_MIN=$4
    PRECISION_STEP=$5
	ABS_ERR_MIN=$6
	ABS_ERR_STEP=$7
    NB_TESTS=$8
    echo "USECASE=${USECASE}"
    echo "RANGE_MIN=${RANGE_MIN}"
    echo "RANGE_STEP=${RANGE_STEP}"
    echo "PRECISION_MIN=${PRECISION_MIN}"
    echo "PRECISION_STEP=${PRECISION_STEP}"
	echo "ABS_ERR_MIN=${ABS_ERR_MIN}"
	echo "ABS_ERR_STEP=${ABS_ERR_STEP}"
    echo "NB_TESTS=${NB_TESTS}"
fi

export VERIFICARLO_BACKEND=VPREC

# Operation parameters
if [ $USECASE = fast ]; then
    operation_list=("+" "x")
else
    operation_list=("+" "-" "/" "x")
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

# Range parameters
declare -A range_max
range_max[0]=8
range_max[1]=11
declare -A range_option
range_option[0]=--range-binary32
range_option[1]=--range-binary64

# Precision parameters
declare -A precision_max
precision_max[0]=23
precision_max[1]=52
declare -A precision_option
precision_option[0]=--precision-binary32
precision_option[1]=--precision-binary64

# Absolute error parameters
declare -A abs_error_max
abs_error_max[0]=-126
abs_error_max[1]=-1022

# Instrumented function parameters
declare -A instrumented_function
instrumented_function[0]="applyOp_float"
instrumented_function[1]="applyOp_double"

retCodeSum=0

for TYPE in "${type_list[@]}"
do
	echo "TYPE: ${type_list_names[${TYPE}]}"

	verificarlo-c -g -Wall test_generative.c --function=${instrumented_function[$TYPE]} -o test_generative -lm

	for MODE in "${modes_list[@]}"
	do
		echo "MODE: ${MODE}"
		for OP in "${operation_list[@]}"
		do 
			echo "OP: ${OP}"
			for RANGE  in `seq ${RANGE_MIN} ${RANGE_STEP} ${range_max[$TYPE]}`
			do
				echo "Range: ${RANGE}"
		    	for PRECISION in `seq ${PRECISION_MIN} ${PRECISION_STEP} ${precision_max[$TYPE]}`
				do
					echo "Precision: ${PRECISION}"
					for ABS_ERR in `seq ${ABS_ERR_MIN} ${ABS_ERR_STEP} ${abs_error_max[$TYPE]}`
					do
						echo "Absolute Error Threshold: ${ABS_ERR}"

						export VFC_BACKENDS="libinterflop_vprec.so ${precision_option[$TYPE]}=${PRECISION} ${range_option[$TYPE]}=${RANGE} --mode=${MODE} --error-mode=abs --max-abs-error-exponent=${ABS_ERR}"
		    			./test_generative $OP $TYPE $NB_TESTS $ABS_ERR
						retCode=$?

						let "retCodeSum+=retCode"
					done
				done
			done
		done
	done

	rm test_generative
done

if [$retCodeSum -eq 0]
then
	echo "Success!"
else
	echo "Failed!"
fi

exit $retCodeSum