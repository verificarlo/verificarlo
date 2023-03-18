#!/bin/bash
set -e

# for EXP in FLOAT FLOAT_POW2 DOUBLE DOUBLE_POW2; do
#   verificarlo-c -D${EXP} -O0 rr_mode.c -o rr_mode_${EXP,,}
# done

parallel --header : "verificarlo-c -D{EXP} -O0 rr_mode.c -o rr_mode_{EXP}" ::: EXP FLOAT FLOAT_POW2 DOUBLE DOUBLE_POW2
exit 1

rm -f run_parallel
export BACKEND=libinterflop_mca.so
for PREC in "--precision-binary32=24" "--precision-binary64=53"; do
  echo Testing $EXP with $BACKEND
  BIN=$PWD/rr_mode_${EXP}
  echo "./compute_error.sh ${BACKEND} ${EXP} ${PREC} ${BIN}" >>run_parallel
done

parallel -j $(nproc) <run_parallel

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
