#!/bin/bash
##uncomment to stop on error
set -e
##uncomment to show all commands executed by the script
#set -x

# Check the number of arguments
if [[ $# != 5 ]]; then
	echo "Expected 5 arguments, $# given"
	echo "Usage: script_name usecase absErr_min absErr_step nb_tests test"
	exit 1
else
	usecase=$1
	abs_err_min=$2
	abs_err_step=$3
	nb_tests=$4
	test=$5

	echo "Usecase: ${usecase}"
	echo "Minimum absolute error: ${abs_err_min}"
	echo "Absolute error step: ${abs_err_step}"
	echo "Number of tests: ${nb_tests}"
	echo "Test: ${test}"
	echo "======================================="
fi

case "$test" in
"additive")
	declare -A abs_error_max=(["float"]=-126 ["double"]=-1022)
	;;
"generative")
	declare -A abs_error_max=(["float"]=-23 ["double"]=-52)
	;;
*)
	echo "Unknown test ${test}"
	exit 1
	;;
esac

# Operation parameters
if [ $usecase = "fast" ]; then
	operation_list=("+" "x")
else
	operation_list=("+" "-" "x" "/")
fi

# Floating-point type list
# type_list=("float" "double")
declare -A type_list=(["float"]=0 ["double"]=1)

# Modes list
if [ $usecase = "fast" ]; then
	modes_list=("OB")
else
	modes_list=("IB" "OB" "FULL")
fi

rm -rf run_parallel

# Move out compilation to faster the test
parallel --header : "make --silent test=${test} type={type}" ::: type ${!type_list[@]}

for type in "${!type_list[@]}"; do
	bin=$(realpath test_${test}_${type})
	for mode in "${modes_list[@]}"; do
		for op in "${operation_list[@]}"; do
			for abs_err in $(seq ${abs_err_min} ${abs_err_step} ${abs_error_max["${type}"]}); do
				echo "./compute_error.sh ${bin} ${mode} ${op} ${type_list[$type]} ${nb_tests} ${abs_err}" >>run_parallel
			done
		done
	done
done

parallel -j $(nproc) <run_parallel

cat >check_status.py <<HERE
import sys
import glob
paths=glob.glob('*.err')
ret=sum([int(open(f).readline().strip()) for f in paths]) if paths != []  else 1
print(ret)
HERE

status=$(python3 check_status.py)

if [ $status -eq 0 ]; then
	echo "Success!"
else
	echo "Failed!"
fi

rm -rf *.err

exit $status
