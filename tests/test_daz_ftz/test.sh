#!/bin/bash
set -e

# Test for the --daz/--ftz options

check_executable() {
    if [[ ! -f $1 ]]; then
        echo "Executable $1 not found"
        exit 1
    fi
}

export VFC_BACKENDS_LOGGER=False
export VFC_BACKENDS_SILENT_LOAD="TRUE"

parallel --header : "verificarlo-c -D REAL={type} -O0 test.c -o test_{type} -lm" ::: type float double
check_executable test_float
check_executable test_double

parallel -j $(nproc) --header : "./compute_error.sh {type} $PWD/test_{type} {backend}" ::: type float double ::: backend libinterflop_mca.so libinterflop_bitmask.so

cat >check_status.py <<HERE
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
