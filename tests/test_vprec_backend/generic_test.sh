#!/bin/bash
##uncomment to stop on error
set -e
##uncomment to show all command executed by the script
#set -x

check_status() {
	if [[ $? != 0 ]]; then
		echo "Error"
		exit 1
	fi
}

export VFC_BACKENDS_LOGGER=False

# Check the number of arguments
if [[ $# -ne 6 ]]; then
	echo "Expected 6 arguments, $# given"
	echo "Usage: script_name usecase range_min range_step precision_min precision_step n_samples"
	exit 1
else
	# Assign arguments to meaningful variable names
	usecase=$1
	range_min=$2
	range_step=$3
	precision_min=$4
	precision_step=$5
	n_samples=$6

	# Print the variable values
	echo "Usecase: ${usecase}"
	echo "Range Min: ${range_min}"
	echo "Range Step: ${range_step}"
	echo "Precision Min: ${precision_min}"
	echo "Precision Step: ${precision_step}"
	echo "Number of Samples: ${n_samples}"
fi
export VERIFICARLO_BACKEND=VPREC

# Operation parameters
if [ "$usecase" = "fast" ]; then
	operation_list=("+" "x")
else
	operation_list=("+" "-" "/" "x")
fi

# Floating type list
float_type_list=("float" "double")

# Modes list
if [ "$usecase" = "fast" ]; then
	modes_list=("OB")
else
	modes_list=("IB" "OB" "FULL")
fi

# Range parameters
declare -A range_max=(["float"]=8 ["double"]=11)
declare -A range_option=(["float"]="--range-binary32" ["double"]="--range-binary64")

rm -f log.error
rm -f run_parallel

parallel --header : "make --silent type={type}" ::: type float double

export COMPUTE_VPREC_ROUNDING=$(realpath compute_vprec_rounding)

for type in "${float_type_list[@]}"; do
	for range in $(seq ${range_min} ${range_step} ${range_max[$type]}); do
		echo "./compute_error.sh $type $range $usecase $range_min $range_step $precision_min $precision_step $n_samples" >>run_parallel
	done
done

parallel -j $(nproc) <run_parallel
# check_status

cat tmp.*/log.error >log.error
# check_status

rm -rf tmp.*

cat log.error

if [ -s "log.error" ]; then
	echo "Test failed"
	exit 1
else
	echo "Test succeeded"
	exit 0
fi
