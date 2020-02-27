#!/bin/bash
set -e

# Test 24 RR for float
for EXP in FLOAT FLOAT_POW2; do
  verificarlo-c -D${EXP} -O0 rr_mode.c -o rr_mode
  for BACKEND in libinterflop_mca.so libinterflop_mca_mpfr.so; do
    export VFC_BACKENDS="$BACKEND --precision-binary32 24 --mode rr"
    rm -f output_${BACKEND}_${EXP}
    for i in `seq 100`; do
      ./rr_mode >> output_${BACKEND}_${EXP}
    done
    echo Testing $EXP with $BACKEND
    diff output_${BACKEND}_${EXP} ref_${EXP}
  done
done

# Test 53 RR for double
for EXP in DOUBLE DOUBLE_POW2; do
  verificarlo-c -D${EXP} -O0 rr_mode.c -o rr_mode
  for BACKEND in libinterflop_mca.so libinterflop_mca_mpfr.so; do
    export VFC_BACKENDS="$BACKEND --precision-binary64 53 --mode rr"
    rm -f output_${BACKEND}_${EXP}
    for i in `seq 100`; do
      ./rr_mode >> output_${BACKEND}_${EXP}
    done
    echo Testing $EXP with $BACKEND
    diff output_${BACKEND}_${EXP} ref_${EXP}
  done
done
