#!/bin/bash
set -e

# Test for the --daz/--ftz options

check_success() {
    if  [[ $? != 0 ]]; then
	echo "Test failed"
	exit 1
    fi	
}

compile() {
    TYPE=$1
    verificarlo-c -D REAL=${TYPE,,} -O0 test.c -o test -lm
    check_success
}

run() {
    TYPE=$1
    IFS=" "    
    while read x y op; do
	./test "$x" "$y" "${op}" >> log_${TYPE^^}
    done < value.${TYPE^^}
    check_success
}

compare() {
    TYPE=$1
    diff -I '#.*' log_${TYPE^^} ref_${TYPE^^}
    check_success
}

export VFC_BACKENDS_SILENT_LOAD="TRUE"

for BACKEND in "libinterflop_mca.so" "libinterflop_mca_mpfr.so" "libinterflop_bitmask.so"; do
    for REALTYPE in "float" "double"; do
	rm -f log_${REALTYPE^^}
	compile $REALTYPE
	for OPTION in " " "--daz" "--ftz" "--daz --ftz"; do
	    export VFC_BACKENDS="$BACKEND --mode=ieee ${OPTION}"
	    run $REALTYPE 	    
	done
	compare $REALTYPE
    done
done

echo "Test succesed"
exit 0
