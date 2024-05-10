#!/bin/bash

set -e

if [[ $# == 1 ]]; then
	usecase=$1
	echo $usecase
fi

case $usecase in
fast)
	range_min=2
	range_step=4
	precision_min=3
	precision_step=20
	n_samples=3
	;;
small)
	range_min=2
	range_step=4
	precision_min=3
	precision_step=20
	n_samples=3
	;;
medium)
	range_min=2
	range_step=2
	precision_min=3
	precision_step=10
	n_samples=3
	;;
full)
	range_min=2
	range_step=1
	precision_min=1
	precision_step=1
	n_samples=3
	;;
cmp)
	./comparison.sh $1 $2 $3
	exit 0
	;;
*)
	usecase=fast
	range_min=2
	range_step=3
	precision_min=3
	precision_step=10
	n_samples=3
	;;
esac

./compile.sh
./generic_test.sh ${usecase} ${range_min} ${range_step} ${precision_min} ${precision_step} ${n_samples}

exit $?
