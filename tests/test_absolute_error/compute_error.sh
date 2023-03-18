#!/bin/bash

BIN=${1}
MODE=${2}
OP=${3}
TYPE=${4}
NB_TESTS=${5}
ABS_ERR=${6}

echo "BIN=${BIN}"
echo "MODE=${MODE}"
echo "OP=${OP}"
echo "TYPE=${TYPE}"
echo "NB_TESTS=${NB_TESTS}"
echo "ABS_ERR=${ABS_ERR}"

export VFC_BACKENDS="libinterflop_vprec.so --mode=${MODE} --error-mode=abs --max-abs-error-exponent=${ABS_ERR}"
$BIN $OP $TYPE $NB_TESTS $ABS_ERR
echo $? >$(mktemp -p .)
