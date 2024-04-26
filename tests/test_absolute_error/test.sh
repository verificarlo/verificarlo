#!/bin/bash

if [[ $# == 1 ]]; then
	usecase=$1
	echo $usecase
fi

case $usecase in
fast)
	range_min=2
	range_step=3
	precision_min=3
	precision_step=10
	abs_err_min=-1
	abs_err_step=-20
	nb_tests=1000
	;;
medium)
	range_min=2
	range_step=2
	precision_min=3
	precision_step=5
	abs_err_min=-1
	abs_err_step=-10
	nb_tests=10000
	;;
full)
	range_min=2
	range_step=1
	precision_min=1
	precision_step=1
	abs_err_min=-1
	abs_err_step=-1
	nb_tests=100000
	;;
*)
	usecase=fast
	range_min=2
	range_step=3
	precision_min=3
	precision_step=10
	abs_err_min=-1
	abs_err_step=-20
	nb_tests=1000
	;;
esac

for TEST in "generative" "additive"; do
	./generic_test_simple.sh ${usecase} ${abs_err_min} ${abs_err_step} ${nb_tests} ${TEST}

	# Check the exit status of the last command
	if [[ $? -ne 0 ]]; then
		echo "generic_test_simple.sh failed for test ${TEST}"
		exit 1
	fi
done

exit 0
