#!/bin/bash
#set -e

export VFC_BACKENDS_SILENT_LOAD=True
export VFC_BACKENDS_LOGGER=False
status=0

rm -f run_parallel
for dtype in "float" "double"; do
  echo $dtype | awk '{ print toupper($0) }'
  verificarlo-c -D REAL=${dtype} -O0 test.c -o test_${dtype}
  for sparsity in 1 0.9 0.5 0.25 0.1 0.01; do
    echo -n " \"./compute_error.sh ${PWD}/test_${dtype} ${sparsity}\" " >>run_parallel
  done
done

RUNS="$(xargs -a run_parallel -0)"
eval "parallel -j $(nproc) -- ${RUNS}"

cat >check_status.py <<HERE
import sys
import glob
paths=glob.glob('tmp.*/output.log')
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
