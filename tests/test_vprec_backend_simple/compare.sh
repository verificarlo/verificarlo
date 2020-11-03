#!/bin/bash

if [[ $# != 6 ]]; then
  echo "usecase: mode range precision type op input_file"
  exit 1
else
  MODE=$1
  RANGE=$2
  PRECISION=$3
  TYPE=$4
  OP=$5
  INPUT_FILE=$6
fi

declare -A precision_option
precision_option["float"]=--precision-binary32
precision_option["double"]=--precision-binary64

declare -A range_option
range_option["float"]=--range-binary32
range_option["double"]=--range-binary64

export VERIFICARLO_PRECISION=$PRECISION
export VERIFICARLO_VPREC_RANGE=$RANGE
export VERIFICARLO_VPREC_TYPE=$TYPE
export VERIFICARLO_VPREC_MODE=$MODE
export VERIFICARLO_OP=$OP
export VFC_BACKENDS="libinterflop_vprec.so ${precision_option[$TYPE]}=${PRECISION} ${range_option[$TYPE]}=${RANGE} --mode=${MODE}"

rm -f compute_vprec_rounding log.error mpfr.txt vprec.txt
verificarlo-c compute_vprec_rounding.c -DREAL=$TYPE -o compute_vprec_rounding

while read a b; do
  ./compute_mpfr_rounding.py $a $b $OP $TYPE >> mpfr.txt
  ./compute_vprec_rounding $a $b $OP >> vprec.txt
done < $INPUT_FILE

./check_output.py 2>> log.error || echo "Output does not match"

if [ -s "log.error" ]
	then
    cat log.error
		echo "Test failed"
		exit 1
	else
		echo "Test suceed"
		exit 0
fi
