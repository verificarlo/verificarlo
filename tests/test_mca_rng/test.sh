#!/bin/bash
##uncomment to stop on error
set -e

PREC_B32=20
PREC_B64=50

SEED=1

#************************
#test the OpenMP version of the test

#compile the test program for testing on floats
verificarlo-c -fopenmp=libomp -D REAL=float -O0 test_openmp.c -o test_openmp_B32
export OMP_NUM_THREADS=4
#set the seed and run the test program
export VFC_BACKENDS="libinterflop_mca.so --mode=rr --precision-binary32=$PREC_B32 --seed=$SEED"
./test_openmp_B32 1>out_run_1 2>log_run_1

#run the test program again
export VFC_BACKENDS="libinterflop_mca.so --mode=rr --precision-binary32=$PREC_B32 --seed=$SEED"
./test_openmp_B32 1>out_run_2 2>log_run_2

#check if the two runs produce identical results
./test_output.py out_run_1 out_run_2

if [ $? -eq 1 ]; then
  echo "test_openmp_B32 failed"
  exit 1
fi

#compile the test program for testing on doubles
verificarlo-c -fopenmp=libomp -D REAL=double -O0 test_openmp.c -o test_openmp_B64

#set the seed and run the test program
export VFC_BACKENDS="libinterflop_mca.so --mode=rr --precision-binary64=$PREC_B64 --seed=$SEED"
./test_openmp_B64 1>out_run_3 2>log_run_3

#run the test program again
export VFC_BACKENDS="libinterflop_mca.so --mode=rr --precision-binary64=$PREC_B64 --seed=$SEED"
./test_openmp_B64 1>out_run_4 2>log_run_4

#check if the two runs produce identical results
./test_output.py out_run_3 out_run_4

if [ $? -eq 1 ]; then
  echo "test_openmp_B64 failed"
  exit 1
fi

#************************
# test the pthread version of the test

#compile the test program for testing on floats
verificarlo-c -D REAL=float -O0 test_pthread.c -o test_pthread_B32 -lpthread

#set the seed and run the test program
export VFC_BACKENDS="libinterflop_mca.so --mode=rr --precision-binary32=$PREC_B32 --seed=$SEED"
./test_pthread_B32 1>out_run_5 2>log_run_5

#run the test program again
export VFC_BACKENDS="libinterflop_mca.so --mode=rr --precision-binary32=$PREC_B32 --seed=$SEED"
./test_pthread_B32 1>out_run_6 2>log_run_6

#check if the two runs produce identical results
./test_output.py out_run_5 out_run_6

if [ $? -eq 1 ]; then
  echo "test_pthread_B32 failed"
  exit 1
fi

#compile the test program for testing on doubles
verificarlo-c -D REAL=double -O0 test_pthread.c -o test_pthread_B64 -lpthread

#set the seed and run the test program
export VFC_BACKENDS="libinterflop_mca.so --mode=rr --precision-binary64=$PREC_B64 --seed=$SEED"
./test_pthread_B64 1>out_run_7 2>log_run_7

#run the test program again
export VFC_BACKENDS="libinterflop_mca.so --mode=rr --precision-binary64=$PREC_B64 --seed=$SEED"
./test_pthread_B64 1>out_run_8 2>log_run_8

#check if the two runs produce identical results
./test_output.py out_run_7 out_run_8

if [ $? -eq 1 ]; then
  echo "test_pthread_B64 failed"
  exit 1
fi

exit 0
