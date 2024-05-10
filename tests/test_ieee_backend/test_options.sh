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
    ./test_options_${TYPE} 2>${LOG} >/dev/null
    echo $LOG
}

check() {
    local TEST=$1
    local TYPE=$2
    local RESULT=$3
    local ERROR_MSG=$4

    if [[ "$RESULT" != 0 ]]; then
        echo "Error:" $ERROR_MSG
        exit 1
    else
        echo "Test $TEST [ok] ($TYPE)"
    fi
}

# Compile tests
parallel --header : "make --silent type={type}" ::: type float double

test1() {
    local id=1
    DEBUG_MODE="--debug"
    OPTIONS=""
    TYPE=$1
    LOG=$(run $TYPE $id $DEBUG_MODE $OPTIONS)
    check $id "$TYPE" "$(
        grep -q "Decimal" ${LOG}
        echo $?
    )" "Error debug mode (Decimal) not printed"
}

test2() {
    local id=2
    DEBUG_MODE="--debug-binary"
    OPTIONS=""
    TYPE=$1
    LOG=$(run $TYPE $id $DEBUG_MODE $OPTIONS)
    check $id "$TYPE" "$(
        grep -q "Binary" ${LOG}
        echo $?
    )" "Error debug mode (Decimal_bin) not printed"
}

test3() {
    local id=3
    DEBUG_MODE="--debug"
    OPTIONS="--no-backend-name"
    TYPE=$1
    LOG=$(run $TYPE $id $DEBUG_MODE $OPTIONS)
    check $id "$TYPE" "$(
        grep -vq "Decimal" ${LOG}
        echo $?
    )" "Error debug mode (Decimal) printed"
}

test4() {
    local id=4
    DEBUG_MODE="--debug-binary"
    OPTIONS="--no-backend-name"
    TYPE=$1
    LOG=$(run $TYPE $id $DEBUG_MODE $OPTIONS)
    check $id "$TYPE" "$(
        grep -vq "Binary" ${LOG}
        echo $?
    )" "Error debug mode (Binary) printed"
}

test5() {
    local id=5
    DEBUG_MODE="--debug"
    OPTIONS="--print-new-line"
    TYPE=$1
    LOG=$(run $TYPE $id $DEBUG_MODE $OPTIONS)
    check $id "$TYPE" "$(
        grep -vq "Decimal" ${LOG}
        echo $?
    )" "Error no new lines printed"
}

test6() {
    local id=6
    DEBUG_MODE="--debug-binary"
    OPTIONS="--print-new-line"
    TYPE=$1
    LOG=$(run $TYPE $id $DEBUG_MODE $OPTIONS)
    check $id "$TYPE" "$(
        test '$(wc -l ${LOG})'
        echo $?
    )" "Error no new lines printed"
}

test7() {
    local id=7
    DEBUG_MODE="--count-op"
    OPTIONS=""
    TYPE=$1
    LOG=$(run $TYPE $id $DEBUG_MODE $OPTIONS)
    check $id "$TYPE" "$(
        grep -vq "add=" ${LOG}
        echo $?
    )" "Error no counts printed"
}

export -f run check
export -f test1 test2 test3 test4 test5 test6 test7

parallel --header : "test{test} {type}" ::: test {1..7} ::: type float double
