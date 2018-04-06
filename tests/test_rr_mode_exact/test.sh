#!/bin/bash
set -e

export VERIFICARLO_PRECISION=24
export VERIFICARLO_MCAMODE=RR

# FLOAT

verificarlo -O0 rr_mode.c -o rr_mode

for BACKEND in MPFR QUAD; do
    export VERIFICARLO_BACKEND=$BACKEND
    rm -f output
    for i in `seq 100`; do
	./rr_mode >> output_$BACKEND
    done
    diff output_$BACKEND ref_FLOAT
done
    
export VERIFICARLO_PRECISION=53
export VERIFICARLO_MCAMODE=RR

# DOUBLE

verificarlo -DDOUBLE -O0 rr_mode.c -o rr_mode

for BACKEND in MPFR QUAD; do
    export VERIFICARLO_BACKEND=$BACKEND
    rm -f output
    for i in `seq 100`; do
	./rr_mode >> output_$BACKEND
    done
    diff output_$BACKEND ref_DOUBLE
done
    
