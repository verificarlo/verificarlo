#!/bin/bash

BIN=${1}
MODE=${2}
OP=${3}
TYPE=${4}
NB_TESTS=${5}
ABS_ERR=${6}

echo
echo "--- Running test ---"
echo "Binary: ${BIN}"
echo "Mode: ${MODE}"
echo "Operation: ${OP}"
echo "Type: ${TYPE}"
echo "Number of tests: ${NB_TESTS}"
echo "Absolute error: ${ABS_ERR}"

# Set the VFC_BACKENDS environment variable
export VFC_BACKENDS="libinterflop_vprec.so --mode=${MODE} --error-mode=abs --max-abs-error-exponent=${ABS_ERR}"

# Run the binary with the given arguments
$BIN $OP $TYPE $NB_TESTS $ABS_ERR

# Write the exit status of the last command to a file
echo $? >"${BIN}_${MODE}_${OP}_${TYPE}_${NB_TESTS}_${ABS_ERR}.err"
