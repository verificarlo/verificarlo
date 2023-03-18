#!/bin/bash
set -e

export VFC_BACKENDS_SILENT_LOAD="true"
export VFC_BACKENDS_LOGGER="true"

run() {
    local TYPE=$1
    local TEST=$2
    local DEBUG_MODE=$3
    local OPTIONS=$4
    local LOG=log_${TEST}_${TYPE}
    export VFC_BACKENDS="libinterflop_ieee.so ${DEBUG_MODE} ${OPTIONS}"
    ./test_options_${TYPE} 2>${LOG}
    echo $LOG
}

check() {
    local RESULT=$1
    local ERROR_MSG=$2

    if [[ "$RESULT" != 0 ]]; then
        echo $ERROR_MSG
        exit 1
    else
        echo "[ok]"
    fi
}

# Compile tests
parallel --header : "verificarlo-c -O0 test_options.c -DREAL={type} -o test_options_{type}" ::: type float double

test1() {
    DEBUG_MODE="--debug"
    OPTIONS=""
    TYPE=$1
    LOG=$(run $TYPE 1 $DEBUG_MODE $OPTIONS)
    check "$(
        grep -q "Decimal" ${LOG}
        echo $?
    )" "Error debug mode (Decimal) not printed"
}

test2() {

    DEBUG_MODE="--debug-binary"
    OPTIONS=""
    TYPE=$1
    LOG=$(run $TYPE 1 $DEBUG_MODE $OPTIONS)
    check "$(
        grep -q "Binary" ${LOG}
        echo $?
    )" "Error debug mode (Decimal_bin) not printed"
}

test3() {

    DEBUG_MODE="--debug"
    OPTIONS="--no-backend-name"
    TYPE=$1
    LOG=$(run $TYPE 1 $DEBUG_MODE $OPTIONS)
    check "$(
        grep -vq "Decimal" ${LOG}
        echo $?
    )" "Error debug mode (Decimal) printed"
}

test4() {

    DEBUG_MODE="--debug-binary"
    OPTIONS="--no-backend-name"
    TYPE=$1
    LOG=$(run $TYPE 1 $DEBUG_MODE $OPTIONS)
    check "$(
        grep -vq "Binary" ${LOG}
        echo $?
    )" "Error debug mode (Binary) printed"
}

test5() {

    DEBUG_MODE="--debug"
    OPTIONS="--print-new-line"
    TYPE=$1
    LOG=$(run $TYPE 1 $DEBUG_MODE $OPTIONS)
    check "$(
        grep -vq "Decimal" ${LOG}
        echo $?
    )" "Error no new lines printed"
}

test6() {

    DEBUG_MODE="--debug-binary"
    OPTIONS="--print-new-line"
    TYPE=$1
    LOG=$(run $TYPE 1 $DEBUG_MODE $OPTIONS)
    check "$(
        test '$(wc -l ${LOG})'
        echo $?
    )" "Error no new lines printed"
}

test7() {

    DEBUG_MODE="--count-op"
    OPTIONS=""
    TYPE=$1
    LOG=$(run $TYPE 1 $DEBUG_MODE $OPTIONS)
    check "$(
        grep -vq "add=" ${LOG}
        echo $?
    )" "Error no counts printed"
}

export -f run check
export -f test1 test2 test3 test4 test5 test6 test7

parallel --header : "test{test} {type}" ::: test {1..7} ::: type float double
