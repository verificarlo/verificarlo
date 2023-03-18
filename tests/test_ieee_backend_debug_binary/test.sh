#!/bin/bash
set -e

# Test for the --debug-binary option

check_success() {
    if [[ $? != 0 ]]; then
        echo "Test failed"
        exit 1
    fi
}

check_executable() {
    if [[ ! -f $1 ]]; then
        echo "Executable $1 not found"
        exit 1
    fi
}

run() {
    TYPE=$1
    if [[ $# == 2 ]]; then
        EXTRA_OPTION='--normalize-denormal'
    else
        EXTRA_OPTION=''
    fi
    IFS=" "
    rm -f log.debug.binary
    while read x y op; do
        ./test_${TYPE} "$x" "$y" "${op}" 2>>log.debug.binary
        echo "$x" "$y" "${op}"
    done <value.${TYPE,,}
    ./generate.py -t ${TYPE,,} -f value.${TYPE,,} ${EXTRA_OPTION} >ref.debug.binary
    check_success
}

compare() {
    diff log.debug.binary ref.debug.binary
    check_success
}

export VFC_BACKENDS_SILENT_LOAD="TRUE"

parallel -j $(nproc) --header : verificarlo -DREAL={type} -O0 test.c --verbose -o test_{type} -lm ::: type float double
check_executable test_float
check_executable test_double

echo "denormal denormalized"
export VFC_BACKENDS="libinterflop_ieee.so --debug-binary --print-new-line --no-backend-name"
for REALTYPE in "float" "double"; do
    run $REALTYPE
    compare
done

echo "denormal normalized"
export VFC_BACKENDS="libinterflop_ieee.so --debug-binary --print-new-line --no-backend-name --print-subnormal-normalized"
for REALTYPE in "float" "double"; do
    run $REALTYPE "denormal"
    compare
done

echo "Test succesed"
exit 0
