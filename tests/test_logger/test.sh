#!/bin/bash
OUTPUT_FILE=output
ERROR_FILE=error

show_env() {
    echo "VFC_BACKENDS_LOGGER=${VFC_BACKENDS_LOGGER}"
    echo "VFC_BACKENDS_SILENT_LOAD=${VFC_BACKENDS_SILENT_LOAD}"
    echo "VFC_BACKENDS_LOGFILE=${VFC_BACKENDS_LOGFILE}"
    echo "VFC_BACKENDS=${VFC_BACKENDS}"
}

reset_env() {
    unset VFC_BACKENDS_LOGGER
    unset VFC_BACKENDS_SILENT_LOAD
    unset VFC_BACKENDS_LOGFILE
    export VFC_BACKENDS="libinterflop_mca.so"
    rm -f $OUTPUT_FILE $ERROR_FILE test.log.*
}

check() {
    RESULT=${1}
    ERROR_MSG=${2}
    if [[ "$RESULT" != 0 ]]; then
        echo "[not ok]"
        show_env
        echo "${ERROR_MSG}"
        exit 1
    else
        echo "[ok]"
    fi
}

check_empty() {
    check "$(
        [ ! -s $1 ]
        echo $?
    )" "${2}"
}

check_non_empty() {
    check "$(
        [ -s $1 ]
        echo $?
    )" "${2}"
}

check_logger_info() {
    check "$(
        grep -q "Info \[verificarlo\]" ${1}
        echo $?
    )" "${2}"
}

check_backend_info() {
    check "$(
        grep -q "Info \[interflop-mcaquad\]" ${1}
        echo $?
    )" "${2}"
}

compile() {
    verificarlo test.c -o test
}

run() {
    ./test >$OUTPUT_FILE 2>$ERROR_FILE
}

compile

echo "* Test 1: Check logger_info is redirected on stderr by default"
reset_env
run
check_logger_info $ERROR_FILE "Error: logger_info not displayed on stderr"
check_backend_info $ERROR_FILE "Error: logger_info not displayed on stderr"

echo "* Test 2: Check VFC_BACKENDS_LOGGER=False disables the logger_info"
reset_env
export VFC_BACKENDS_LOGGER=False
run
check_empty $OUTPUT_FILE "Error: logger displayed on stdout"
check_empty $ERROR_FILE "Error: logger displayed on stderr"

echo "* Test 3: Check VFC_BACKENDS_LOGFILE='test.log' redirects logger_info into test.log"
reset_env
export VFC_BACKENDS_LOGGER=True
export VFC_BACKENDS_LOGFILE='test.log'
run
check_empty $OUTPUT_FILE "Error: logger_info displayed on stdout"
check_empty $ERROR_FILE "Error: logger_info displayed on stderr"
check_logger_info test.log.* "Error: logger_info not displayed on test.log"
check_backend_info test.log.* "Error: logger_info not displayed on test.log"

echo "* Test 4: Check logger_error is displayed when an error occured and VFC_BACKENDS_LOGFILE is set"
reset_env
export VFC_BACKENDS_LOGGER=True
export VFC_BACKENDS=""
export VFC_BACKENDS_LOGFILE="test.log"
run
check_empty $OUTPUT_FILE "Error: logger_error displayed on stdout"
check_empty test.log.* "Error: logger_error displayed on test.log"
check_non_empty $ERROR_FILE "Error: logger_error not displayed on stderr"

echo "* Test 5: Check logger_error is displayed when an error occured and VFC_BACKENDS_LOGGER=False"
reset_env
export VFC_BACKENDS_LOGGER=False
export VFC_BACKENDS=""
run
check_empty $OUTPUT_FILE "Error: logger_error displayed on stdout"
check_empty test.log.* "Error: logger_error displayed on test.log"
check_non_empty $ERROR_FILE "Error: logger_error not displayed on stderr"
