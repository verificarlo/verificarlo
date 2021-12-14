#!/usr/bin/env bash

TYPE=$1
PREC=$2
MODE=$3
SEED=$4
BIN=$5
OP=$6

TMPDIR=$(mktemp -d -p .)
cd $TMPDIR

declare -A options
options[float]='--precision-binary32'
options[double]='--precision-binary64'

echo "Checking at PRECISION $PREC MODE $MODE"
rm -f out_mpfr out_quad
export VFC_BACKENDS="libinterflop_mca_mpfr.so ${options[$TYPE]}=$PREC --mode $MODE --seed=$SEED"
$BIN $OP >out_mpfr
# MPFR returns specific nan (FFC00000) when the result is a NaN
# we must remove the negative sign to not break the diff comparison
sed -i "s\-nan\nan\g" out_mpfr

export VFC_BACKENDS="libinterflop_mca.so ${options[$TYPE]}=$PREC --mode $MODE --seed=$SEED"
$BIN $OP >out_quad
sed -i "s\-nan\nan\g" out_quad

diff out_mpfr out_quad >log
echo $? >output.txt
