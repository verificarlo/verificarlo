#!/bin/bash
set -e

# Test for the --daz/--ftz options

export VFC_BACKENDS_LOGGER=False
export VFC_BACKENDS_SILENT_LOAD="TRUE"

parallel --header : "make --silent type={type}" ::: type float double

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
