#!/bin/bash
set -e
export VERIFICARLO_PRECISION=24
export VERIFICARLO_MCAMODE=RR

for EXP in FLOAT FLOAT_POW2; do
  verificarlo -D${EXP} -O0 rr_mode.c -o rr_mode
  for BACKEND in MPFR QUAD; do
    export VERIFICARLO_BACKEND=${BACKEND}
    rm -f output_${BACKEND}_${EXP}
    for i in `seq 100`; do
      ./rr_mode >> output_${BACKEND}_${EXP}
    done
    echo Testing $EXP with $BACKEND
    diff output_${BACKEND}_${EXP} ref_${EXP}
  done
done

export VERIFICARLO_PRECISION=53
export VERIFICARLO_MCAMODE=RR

for EXP in DOUBLE DOUBLE_POW2; do
  verificarlo -D${EXP} -O0 rr_mode.c -o rr_mode
  for BACKEND in MPFR QUAD; do
    export VERIFICARLO_BACKEND=${BACKEND}
    rm -f output_${BACKEND}_${EXP}
    for i in `seq 1000`; do
      ./rr_mode >> output_${BACKEND}_${EXP}
    done
    echo Testing $EXP with $BACKEND
    diff output_${BACKEND}_${EXP} ref_${EXP}
  done
done
