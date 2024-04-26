#!/bin/bash
set -e

experiments=(FLOAT FLOAT_POW2 DOUBLE DOUBLE_POW2)
precisions=("--precision-binary32=24" "--precision-binary64=53")

parallel --header : "make --silent exp={exp}" ::: exp ${experiments[@]}

backends=libinterflop_mca.so
parallel -j $(nproc) --header : "./compute_error.sh ${backends} {exp} {prec} ${PWD}/rr_mode_{exp} " ::: exp ${experiments[@]} ::: prec ${precisions[@]}

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
