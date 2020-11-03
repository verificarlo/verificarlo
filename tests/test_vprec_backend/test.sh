#!/bin/bash

if [[ $# == 1 ]]; then
    usecase=$1
    echo $usecase
fi

case $usecase in
    fast)
	RANGE_MIN=2
	RANGE_STEP=4
	PRECISION_MIN=3
	PRECISION_STEP=20
	N_SAMPLES=3
	;;
    small)
	RANGE_MIN=2
	RANGE_STEP=4
	PRECISION_MIN=3
	PRECISION_STEP=20
	N_SAMPLES=3
	;;
    medium)
	RANGE_MIN=2
	RANGE_STEP=2
	PRECISION_MIN=3
	PRECISION_STEP=10
	N_SAMPLES=3
	;;
    full)
	RANGE_MIN=2
	RANGE_STEP=1
	PRECISION_MIN=1
	PRECISION_STEP=1
	N_SAMPLES=3
	;;
    cmp) ./comparison.sh $1 $2 $3; exit 0 ;;
    *)
	usecase=fast
	RANGE_MIN=2
	RANGE_STEP=3
	PRECISION_MIN=3
	PRECISION_STEP=10
	N_SAMPLES=3
esac

./generic_test.sh ${usecase} ${RANGE_MIN} ${RANGE_STEP} ${PRECISION_MIN} ${PRECISION_STEP} ${N_SAMPLES}

exit $?
