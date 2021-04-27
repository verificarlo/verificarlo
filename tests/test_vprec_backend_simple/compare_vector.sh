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

rm -f compute_vprec_rounding_vector log.error mpfr_vector.txt vprec_vector.txt
verificarlo-c -DREAL=$TYPE -march=native compute_vprec_rounding_vector.c -o compute_vprec_rounding_vector

while read a1 b1 && read a2 b2; do
    ./compute_mpfr_rounding.py $a1 $b1 $OP $TYPE >> mpfr_vector.txt
    ./compute_mpfr_rounding.py $a2 $b2 $OP $TYPE >> mpfr_vector.txt
    ./compute_vprec_rounding_vector $a1 $b1 $a2 $b2 $OP >> vprec_vector.txt
done < $INPUT_FILE

./check_output.py "v" 2>> log.error || echo "Output does not match"

if [ -s "log.error" ]
        then
    cat log.error
                echo "Test failed"
                exit 1
        else
                echo "Test suceed"
                exit 0
fi
