#!/usr/bin/env bash

BACKEND=$1
EXP=$2
PREC=$3
BIN=$4

TMPDIR=$(mktemp -d -p .)
cd $TMPDIR

export VFC_BACKENDS="$BACKEND ${PREC} --mode rr"
for i in $(seq 100); do
    export VFC_BACKENDS_LOGFILE="tmp"
    ${BIN} >>output_${BACKEND}_${EXP}
done
echo Testing $EXP with $BACKEND
diff output_${BACKEND}_${EXP} ../ref_${EXP}
echo $? >output.txt
