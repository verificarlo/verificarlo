#!/bin/bash
##uncomment to stop on error
#set -e

PREC_B32=20
PREC_B64=50

SEED=1

# compile the test program for testing on floats
verificarlo-c -fopenmp=libiomp5 -D REAL=float -O0 test.c -o test

# set the seed and run the test program
export VFC_BACKENDS="libinterflop_mca_mpfr.so --mode=rr --precision-binary32=$PREC_B32 --seed=$SEED --rng-mode=rand"
./test 1> out_run_1 2> log_run_1

# run the test program again
export VFC_BACKENDS="libinterflop_mca_mpfr.so --mode=rr --precision-binary32=$PREC_B32 --seed=$SEED --rng-mode=rand"
./test 1> out_run_2 2> log_run_2

# check if the two runs produce identical results
./test_output.py out_run_1 out_run_2

exit $?