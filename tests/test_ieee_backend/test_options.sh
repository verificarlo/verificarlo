#!/bin/bash
set -e

export VFC_BACKENDS_SILENT_LOAD="true"
export VFC_BACKENDS_LOGGER="false"

run() {
    BINARY=$1

    export VFC_BACKENDS="libinterflop_ieee.so ${DEBUG_MODE} ${OPTIONS}"
    echo -e "\n### ${VFC_BACKENDS}"
    ./$BINARY 2> log
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

# Scalar type
for TYPE in float double ; do

    echo -e "\n$TYPE"

    BINARY=test_options

    verificarlo-c -O0 -march=native test_options.c -DREAL=$TYPE -o $BINARY

    DEBUG_MODE="--debug"
    OPTIONS=""
    run $BINARY
    check "$(grep -q "Decimal" log; echo $?)" "Error debug mode (Decimal) not printed"

    DEBUG_MODE="--debug-binary"
    OPTIONS=""
    run $BINARY  
    check "$(grep -q "Binary" log; echo $?)" "Error debug mode (Decimal_bin) not printed"

    DEBUG_MODE="--debug"
    OPTIONS="--no-backend-name"
    run $BINARY
    check "$(grep -vq "Decimal" log; echo $?)" "Error debug mode (Decimal) printed"

    DEBUG_MODE="--debug-binary"
    OPTIONS="--no-backend-name"
    run $BINARY
    check "$(grep -vq "Binary" log; echo $?)" "Error debug mode (Binary) printed"

    DEBUG_MODE="--debug"
    OPTIONS="--print-new-line"
    run $BINARY
    check "$(grep -vq "Decimal" log; echo $?)" "Error no new lines printed"

    DEBUG_MODE="--debug-binary"
    OPTIONS="--print-new-line"
    run $BINARY
    check "$(test '$(wc -l log)'; echo $?)" "Error no new lines printed"
    
done

# Vector type
for TYPE in float4 double2 ; do

    echo -e "\n$TYPE"

    BINARY=test_options_vector

    verificarlo-c -O0 -march=native test_options_vector.c -DREAL=$TYPE -o $BINARY

    DEBUG_MODE="--debug"
    OPTIONS=""
    run $BINARY
    check "$(grep -q "Decimal" log; echo $?)" "Error debug mode (Decimal) not printed"

    DEBUG_MODE="--debug-binary"
    OPTIONS=""
    run $BINARY  
    check "$(grep -q "Binary" log; echo $?)" "Error debug mode (Decimal_bin) not printed"

    DEBUG_MODE="--debug"
    OPTIONS="--no-backend-name"
    run $BINARY
    check "$(grep -vq "Decimal" log; echo $?)" "Error debug mode (Decimal) printed"

    DEBUG_MODE="--debug-binary"
    OPTIONS="--no-backend-name"
    run $BINARY
    check "$(grep -vq "Binary" log; echo $?)" "Error debug mode (Binary) printed"

    DEBUG_MODE="--debug"
    OPTIONS="--print-new-line"
    run $BINARY
    check "$(grep -vq "Decimal" log; echo $?)" "Error no new lines printed"

    DEBUG_MODE="--debug-binary"
    OPTIONS="--print-new-line"
    run $BINARY
    check "$(test '$(wc -l log)'; echo $?)" "Error no new lines printed"

    DEBUG_MODE="--count-op"
    OPTIONS=""
    run
    check "$(grep -vq "add=" log; echo $?)" "Error no counts printed"

done
