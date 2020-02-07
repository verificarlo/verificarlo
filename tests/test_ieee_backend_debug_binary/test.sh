#!/bin/bash
#set -e

# Test for the --debug-binary option
# TODO: add a test for the --debug option

check_success() {
    if  [[ $? != 0 ]]; then
	echo "Test failed"
	exit 1
    fi	
}

compile() {
    TYPE=$1
    verificarlo -D${TYPE^^} -O0 test.c --verbose -o test -lm
    check_success
}

run() {
    TYPE=$1
    IFS=" "    
    rm -f log.debug.binary
    while read x y op; do
	./test "$x" "$y" "${op}" 2>> log.debug.binary
	echo "$x" "$y" "${op}"
    done < value.${TYPE,,}
    ./generate.py ${TYPE,,} value.${TYPE,,} > ref.debug.binary
    check_success
}

compare() {
    diff log.debug.binary ref.debug.binary
    check_success
}

export VFC_BACKENDS_SILENT_LOAD="TRUE"
export VFC_BACKENDS="libinterflop_ieee.so --debug-binary --print-new-line --no-print-debug-mode"

for REALTYPE in "float" "double"; do
    compile $REALTYPE
    run $REALTYPE
    compare
done

echo "Test succesed"
exit 0
