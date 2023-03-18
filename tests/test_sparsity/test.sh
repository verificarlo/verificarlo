#!/bin/bash
#set -e

export VFC_BACKENDS_SILENT_LOAD=True
export VFC_BACKENDS_LOGGER=False
status=0

parallel --header : "verificarlo-c -O0 -DREAL={type} test.c -o test_{type}" ::: type float double

# rm -f run_parallel
# for dtype in "float" "double"; do
#   for sparsity in 1 0.9 0.5 0.25 0.1 0.01; do
#     echo "./compute_error.sh ${PWD}/test_${dtype} ${sparsity}" >>run_parallel
#   done
# done

parallel -j $(nproc) --header : ./compute_error.sh test_{type} {sparsity} ::: type float double ::: sparsity 1 0.9 0.5 0.25 0.1 0.01

cat >check_status.py <<HERE
import sys
import glob
paths=glob.glob('tmp*')
ret=sum([int(open(f).readline().strip()) for f in paths]) if paths != [] else 1
print(ret)
HERE

status=$(python3 check_status.py)

if [ $status -eq 0 ]; then
  echo "Success!"
else
  echo "Failed!"
fi

# rm -rf tmp.*

exit $status
