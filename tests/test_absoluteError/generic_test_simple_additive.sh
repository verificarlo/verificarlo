#!/bin/bash
##uncomment to stop on error
#set -e
##uncomment to show all commands executed by the script
#set -x

if [[ $# != 4 ]]; then
    echo "expected 4 arguments, $# given"
    echo "usecase absErr_min absErr_step nb_tests"
    exit 1
else
    USECASE=$1
    ABS_ERR_MIN=$2
	ABS_ERR_STEP=$3
    NB_TESTS=$4
    echo "USECASE=${USECASE}"
    echo "ABS_ERR_MIN=${ABS_ERR_MIN}"
	echo "ABS_ERR_STEP=${ABS_ERR_STEP}"
    echo "NB_TESTS=${NB_TESTS}"
fi

export VERIFICARLO_BACKEND=VPREC

# Operation parameters
if [ $USECASE = fast ]; then
    operation_list=("+" "*")
else
    operation_list=("+" "-" "*" "/")
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

# Absolute error parameters
declare -A abs_error_max
abs_error_max[0]=-126
abs_error_max[1]=-1022
# abs_error_max[0]=-23
# abs_error_max[1]=-52

# Instrumented function parameters
declare -A instrumented_function
instrumented_function[0]="applyOp_float"
instrumented_function[1]="applyOp_double"

retCodeSum=0

for TYPE in "${type_list[@]}"
do
	echo "TYPE: ${type_list_names[${TYPE}]}"

	verificarlo-c -g -Wall test_additive.c --function=${instrumented_function[${TYPE}]} -o test_additive -lm

	for MODE in "${modes_list[@]}"
	do
		echo "MODE: ${MODE}"
		for OP in "${operation_list[@]}"
		do 
			echo "OP: ${OP}"
			for ABS_ERR in `seq ${ABS_ERR_MIN} ${ABS_ERR_STEP} ${abs_error_max[${TYPE}]}`
			do
				echo "Absolute Error Threshold: ${ABS_ERR}"
				
				export VFC_BACKENDS="libinterflop_vprec.so --mode=${MODE} --error-mode=abs --max-abs-error-exponent=${ABS_ERR}"
				./test_additive "${OP}" $TYPE $NB_TESTS $ABS_ERR
				retCode=$?

				let "retCodeSum+=retCode"
			done
		done
	done

	rm test_additive
done

if [ $retCodeSum -eq 0 ]
then
	echo "Success!"
else
	echo "Failed!"
fi

exit $retCodeSum