#!/bin/bash

if [[ $# == 1 ]]; then
    usecase=$1
    echo $usecase
fi

case $usecase in
    small)	
	RANGE_MIN=2
	RANGE_STEP=4
	PRECISION_MIN=3
	PRECISION_STEP=20
	;;
    medium)
	RANGE_MIN=2
	RANGE_STEP=2
	PRECISION_MIN=3
	PRECISION_STEP=10
	;;
    full)
	RANGE_MIN=1
	RANGE_STEP=1
	PRECISION_MIN=1
	PRECISION_STEP=1
	;;
    cmp) ./comparison.sh $1 $2 $3; exit 0 ;;
    *)
	RANGE_MIN=2
	RANGE_STEP=4
	PRECISION_MIN=3
	PRECISION_STEP=20
esac

    echo "RANGE_MIN=${RANGE_MIN}"
    echo "RANGE_STEP=${RANGE_STEP}"
    echo "PRECISION_MIN=${PRECISION_MIN}"
    echo "PRECISION_STEP=${PRECISION_STEP}"


./generic_test.sh ${RANGE_MIN} ${RANGE_STEP} ${PRECISION_MIN} ${PRECISION_STEP}
  
