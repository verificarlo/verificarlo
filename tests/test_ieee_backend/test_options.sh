#!/bin/bash
set -e

export VFC_BACKENDS_SILENT_LOAD="true"

run() {
    export VFC_BACKENDS="libinterflop_ieee.so ${DEBUG_MODE} ${OPTIONS}"
    echo -e "\n### ${VFC_BACKENDS}"
    ./test_options 2> log
}

eval_condition() {
    
    if [[ $? != 0 ]]; then
	echo 0
    else
	echo 1
    fi
}

check() {
  
    RESULT=$1
    ERROR_MSG=$2
    
    if [[ "$RESULT" != 0 ]] ; then
       echo $ERROR_MSG
       exit 1
    else
	echo "[ok]"
    fi
}

for TYPE in float double; do

    verificarlo -O0 test_options.c -DREAL=float -o test_options

    DEBUG_MODE="--debug"
    OPTIONS=""
    run
    check "$(grep -q "interflop_ieee" log; echo $?)" "Error debug mode (interflop_ieee) not printed"

    DEBUG_MODE="--debug-binary"
    OPTIONS=""
    run   
    check "$(grep -q "interflop_ieee_bin" log; echo $?)" "Error debug mode (interflop_ieee_bin) not printed"

    DEBUG_MODE="--debug"
    OPTIONS="--no-backend-name"
    run
    check "$(grep -vq "interflop_ieee" log; echo $?)" "Error debug mode (interflop_ieee) printed"

    DEBUG_MODE="--debug-binary"
    OPTIONS="--no-backend-name"
    run
    check "$(grep -vq "interflop_ieee_bin" log; echo $?)" "Error debug mode (interflop_ieee_bin) printed"

    DEBUG_MODE="--debug"
    OPTIONS="--print-new-line"
    run
    check "$(grep -vq "interflop_ieee" log; echo $?)" "Error no new lines printed"

    DEBUG_MODE="--debug-binary"
    OPTIONS="--print-new-line"
    run
    check "$(test '$(wc -l log)'; echo $?)" "Error no new lines printed"
    
done
