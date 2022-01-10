#!/usr/bin/env bash

TYPE=$1
OPERATOR=$2
MODE=$3
PREC=$4
SEED=$5
SAMPLES=$6
OP=$7

echo "TYPE=${TYPE}"
echo "OPERATOR=${OPERATOR}"
echo "MODE=${MODE}"
echo "PREC=${PREC}"
echo "SEED=${SEED}"
echo "SAMPLES=${SAMPLES}"
echo "OP=${OP}"

declare -A options
options[FLOAT]=--precision-binary32
options[DOUBLE]=--precision-binary64

declare -A precision
precision[FLOAT]=24
precision[DOUBLE]=53

declare -A bitmask_modes
declare -A mca_modes
bitmask_modes=([IB]="ib" [OB]="ob" [FULL]="full")
mca_modes=([IB]="pb" [OB]="rr" [FULL]="mca")

compare() {
    S_MCA=$(cat $1)
    S_BITMASK=$(cat $2)
    T=$3
    ../compare.py --s-mca $S_MCA --s-bitmask $S_BITMASK --virtual-precision=$3
    if [ $? -ne 0 ]; then
        echo "error: |${S_MCA}-${S_BITMASK}| > 2"
        exit 1
    else
        echo "ok for precision $PREC"
        echo "|${S_MCA}-${S_BITMASK}| <= 2"
    fi
}

TMPDIR=$(mktemp -d -p .)
cd $TMPDIR

export VFC_BACKENDS="libinterflop_mca.so ${options[$TYPE]}=$PREC --mode=${mca_modes[$MODE]} --seed=$SEED"
echo $VFC_BACKENDS
../test_${TYPE,,} ${OP} >out_mca
../compute_sig_${TYPE,,} $SAMPLES out_mca >s_mca

export VFC_BACKENDS="libinterflop_bitmask.so ${options[$TYPE]}=$PREC --operator=${OPERATOR} --mode=${bitmask_modes[$MODE]} --seed=${SEED}"
echo $VFC_BACKENDS
../test_${TYPE,,} ${OP} >out_bitmask
../compute_sig_${TYPE,,} $SAMPLES out_bitmask >s_bitmask

compare s_mca s_bitmask $PREC
echo $? >output.txt
