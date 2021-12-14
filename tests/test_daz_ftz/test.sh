#!/bin/bash
set -e

# Test for the --daz/--ftz options

check_success() {
    if [[ $? != 0 ]]; then
        echo "Test failed"
        exit 1
    fi
}

compile() {
    TYPE=$1
    verificarlo-c -D REAL=${TYPE,,} -O0 test.c -o test_${TYPE,,} -lm
    check_success
}

run() {
    TYPE=$1
    IFS=" "
    while read x y op; do
        ./test "$x" "$y" "${op}" >>log_${TYPE^^}
    done <value.${TYPE^^}
    check_success
}

compare() {
    TYPE=$1
    diff -I '#.*' log_${TYPE^^} ref_${TYPE^^}
    check_success
}

export VFC_BACKENDS_SILENT_LOAD="TRUE"

rm -f run_parallel
for REALTYPE in "float" "double"; do
    compile $REALTYPE
    for BACKEND in "libinterflop_mca.so" "libinterflop_mca_mpfr.so" "libinterflop_bitmask.so"; do
        echo -n " \"./compute_error.sh ${TYPE} ${PWD}/test_${TYPE,,} ${BACKEND} \"" >>run_parallel
    done
done

RUNS="$(xargs -a run_parallel -0)"
eval "parallel -j $(nproc) -- ${RUNS}"

cat >check_status.py <<HERE
import sys
import glob
paths=glob.glob('tmp.*/output.txt')
ret=sum([int(open(f).readline().strip()) for f in paths]) if paths != [] else 1
print(ret)
HERE

status=$(python3 check_status.py)

if [ $status -eq 0 ]; then
    echo "Success!"
else
    echo "Failed!"
fi

rm -rf tmp.*

exit $status
