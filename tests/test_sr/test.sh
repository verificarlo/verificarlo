#!/bin/bash
set -e

rm -rf logs/ binary32-* binary64-* sr
mkdir logs/
export VFC_BACKENDS_LOGFILE="logs/verificarlo.log"
export BACKEND="libinterflop_mca.so"

verificarlo-c -O0 sr-binary32.c -o sr-binary32 -lm
verificarlo-c -O0 sr-binary64.c -o sr-binary64 -lm

for p in 23 24 25 26; do
  for PREC in "--mode=rr --precision-binary32=24"; do
    VFC_BACKENDS="$BACKEND $PREC" ./sr-binary32 $p > binary32-$p
  done
done

for p in 52 53 54 55; do
  for PREC in "--mode=rr --precision-binary64=53"; do
    VFC_BACKENDS="$BACKEND $PREC" ./sr-binary64 $p > binary64-$p
  done
done

./check.py
status=$?

if [ $status -eq 0 ]; then
  echo "Success!"
else
  echo "Failed!"
fi

exit $status
