#!/bin/bash
#set -e


SEED=$RANDOM
echo "Seed: $SEED"

declare -A options
options[float]='--precision-binary32'
options[double]='--precision-binary64'

declare -A types
options[float]='float'
options[double]='double'


# ../../build/bin/verificarlo-c -fopenmp=libiomp5 -D REAL=float -O0 test.c -o test
../../build/bin/verificarlo-c -fopenmp=libiomp5 -D REAL=double -O0 test.c -o test