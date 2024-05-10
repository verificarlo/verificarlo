#!/bin/bash
#set -e

check_status() {
  if [[ $? != 0 ]]; then
    echo "Error"
    exit 1
  fi
}

export VFC_BACKENDS_SILENT_LOAD=True
export VFC_BACKENDS_LOGGER=False
status=0

parallel --header : "verificarlo-c -O0 -DREAL={type} test.c -o test_{type}" ::: type float double
check_status

parallel -k -j $(nproc) --header : ./compute_error.sh test_{type} {sparsity} ::: type float double ::: sparsity 1 0.9 0.5 0.25 0.1 0.01
check_status

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

rm -rf tmp.*

exit $status
