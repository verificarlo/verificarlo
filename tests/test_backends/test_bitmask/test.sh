#!/bin/bash

getprec() {
    if [ $type == FLOAT ]; then
	echo 24
    elif [ $type == DOUBLE ]; then
	echo 53
    fi
}

Run_bitmask() {
    max_prec=$(getprec $1)
    echo $max_prec
    bitmask_mode=$2

    rm -f bitmask_out

    export VERIFICARLO_BACKEND="BITMASK"
    export VERIFICARLO_BITMASK_MODE="${bitmask_mode}"

    ./test_bitmask_ref

    for i in `seq 1 ${max_prec}`; do
	export VERIFICARLO_PRECISION=$i
	echo -n  "${i} " >> bitmask_out
	./test_bitmask >> bitmask_out
    done;
}

Run_MPFR() {
    MAX_PREC=$1
    NB_SAMPLE=$2
    REAL_TYPE=$3
    export VERIFICARLO_BACKEND="MPFR"
    for p in `seq 1 ${MAX_PREC}`; do
	rm -f mpfr_out
	export VERIFICARLO_PRECISION=$p
	for i in `seq 1 ${NB_SAMPLE}`; do
	    echo -n  "${i} " >> mpfr_out
	    ./test_bitmask >> mpfr_out
	done;
	bitmask_FP=$(grep "^${p} " bitmask_out)

	ret=$(./compare.py mpfr_out "${bitmask_FP}" $REAL_TYPE)
	if [ "${ret}" = "False" ]; then
	    echo "Test failed"
	    exit 1
	fi
    done;
}

Check() {
    ref=$1
    file=$2
    diff ${ref} ${file}
    if [ $? != 0 ]; then
	echo "Test failed"
	echo "difference between ${ref} ${file}"
	exit 1
    else
	echo "Test passed"
    fi
}

Compile() {
    type=$1
    bitmask_mode=$2
    macro_flags="-D ${type} -D ${bitmask_mode}"

    echo "Compute " $type " bitmask_mode : " $bitmask_mode 

    verificarlo $macro_flags test_bitmask_ref.c -o test_bitmask_ref
    verificarlo $macro_flags test_bitmask.c -o test_bitmask
}

for rtype in FLOAT DOUBLE; do
    for bm_mode in ZERO INV; do
	Compile $rtype $bm_mode
	Run_bitmask $rtype $bm_mode
	cp bitmask_ref bitmask_${rtype}_${bm_mode}_ref
	cp bitmask_out bitmask_${rtype}_${bm_mode}_out
	Check bitmask_${rtype}_${bm_mode}_ref  bitmask_${rtype}_${bm_mode}_out
	# echo "Compare with MPFR"
	# Run_MPFR 24 100 "FLOAT"
    done;
done;
