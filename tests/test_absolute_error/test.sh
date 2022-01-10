#!/bin/bash

if [[ $# == 1 ]]; then
	usecase=$1
	echo $usecase
fi

case $usecase in
fast)
	RANGE_MIN=2
	RANGE_STEP=3
	PRECISION_MIN=3
	PRECISION_STEP=10
	ABS_ERR_MIN=-1
	ABS_ERR_STEP=-20
	NB_TESTS=1000
	;;
medium)
	RANGE_MIN=2
	RANGE_STEP=2
	PRECISION_MIN=3
	PRECISION_STEP=5
	ABS_ERR_MIN=-1
	ABS_ERR_STEP=-10
	NB_TESTS=10000
	;;
full)
	RANGE_MIN=2
	RANGE_STEP=1
	PRECISION_MIN=1
	PRECISION_STEP=1
	ABS_ERR_MIN=-1
	ABS_ERR_STEP=-1
	NB_TESTS=100000
	;;
*)
	usecase=fast
	RANGE_MIN=2
	RANGE_STEP=3
	PRECISION_MIN=3
	PRECISION_STEP=10
	ABS_ERR_MIN=-1
	ABS_ERR_STEP=-20
	NB_TESTS=1000
	;;
esac

for TEST in "generative" "additive"; do
	./generic_test_simple.sh ${usecase} ${ABS_ERR_MIN} ${ABS_ERR_STEP} ${NB_TESTS} ${TEST}
done

exit $?
