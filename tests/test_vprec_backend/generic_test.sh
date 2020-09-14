#!/bin/bash
##uncomment to stop on error
#set -e
##uncomment to show all command executed by the script
#set -x

if [[ $# != 6 ]]; then
    echo "expected 5 arguments, $# given"
    echo "usecase range_min range_step precision_min precision_step n_samples"
    exit 1
else
    USECASE=$1
    RANGE_MIN=$2
    RANGE_STEP=$3
    PRECISION_MIN=$4
    PRECISION_STEP=$5
    N_SAMPLES=$6
    echo "USECASE=${USECASE}"
    echo "RANGE_MIN=${RANGE_MIN}"
    echo "RANGE_STEP=${RANGE_STEP}"
    echo "PRECISION_MIN=${PRECISION_MIN}"
    echo "PRECISION_STEP=${PRECISION_STEP}"
    echo "N_SAMPLES=${N_SAMPLES}"
fi

export VERIFICARLO_BACKEND=VPREC

# Operation parameters
if [ $USECASE = fast ]; then
    operation_list=("+" "x")
else
    operation_list=("+" "-" "/" "x")
fi

# Floating type list
float_type_list=("float" "double")

# Modes list
if [ $USECASE = "fast" ]; then
    modes_list=("OB")
else
    modes_list=("IB" "OB" "FULL")
fi

# Range parameters
declare -A range_max
range_max["float"]=8
range_max["double"]=11
declare -A range_option
range_option["float"]=--range-binary32
range_option["double"]=--range-binary64

# Precision parameters
declare -A precision_max
precision_max["float"]=23
precision_max["double"]=52
declare -A precision_option
precision_option["float"]=--precision-binary32
precision_option["double"]=--precision-binary64

rm -f log.error

print_sep() {
    LENGTH=$(expr length "$1")
    printf '#%.0s' `seq ${LENGTH}`
    printf "\n"
}

compute_op() {
  while read a b; do
    ./compute_mpfr_rounding.py $a $b $1 $2 >> mpfr.txt
    ./compute_vprec_rounding $a $b $1 >> vprec.txt
  done < input.txt
}

for TYPE in "${float_type_list[@]}"
do
	echo "TYPE: ${TYPE}"
	export VERIFICARLO_VPREC_TYPE=$TYPE
    	verificarlo-c compute_vprec_rounding.c -DREAL=$TYPE -o compute_vprec_rounding --verbose
    	for MODE in "${modes_list[@]}"
	do
		echo "MODE: ${MODE}"
		export VERIFICARLO_VPREC_MODE=$MODE
		for RANGE  in `seq ${RANGE_MIN} ${RANGE_STEP} ${range_max[$TYPE]}`
		do
	    		echo "Range: ${RANGE}"
	    		export VERIFICARLO_VPREC_RANGE=$RANGE
			rm -f input.txt
			./generate_input.py $N_SAMPLES $RANGE
			for OP in "${operation_list[@]}"
			do 
				echo "OP: ${OP}"
		    		export VERIFICARLO_OP=$OP
				for PRECISION in `seq ${PRECISION_MIN} ${PRECISION_STEP} ${precision_max[$TYPE]}`
				do
		    			echo "Precision: ${PRECISION}"
		    			export VERIFICARLO_PRECISION=$PRECISION
		    			export VFC_BACKENDS="libinterflop_vprec.so ${precision_option[$TYPE]}=${PRECISION} ${range_option[$TYPE]}=${RANGE} --mode=${MODE}"
		    			rm -f mpfr.txt vprec.txt
					compute_op $OP $TYPE
		    			./check_output.py 2>> log.error
				done
      done
		done
		rm compute_vprec_rounding
	done
done

cat log.error

if [ -s "log.error" ]
	then
		echo "Test failed"
		exit 1
	else
		echo "Test suceed"
		exit 0
fi
