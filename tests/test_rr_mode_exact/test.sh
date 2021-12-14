#!/bin/bash
set -e

for EXP in FLOAT FLOAT_POW2 DOUBLE DOUBLE_POW2; do
  verificarlo-c -D${EXP} -O0 rr_mode.c -o rr_mode_${EXP,,}
done

rm -f run_parallel
for BACKEND in libinterflop_mca.so libinterflop_mca_mpfr.so; do
  for PREC in "--precision-binary32=24" "--precision-binary64=53"; do
    echo Testing $EXP with $BACKEND
    BIN=$PWD/rr_mode_${EXP,,}
    echo -n " \" ./compute_error.sh ${BACKEND} ${EXP} ${PREC} ${BIN} \" " >>run_parallel
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
