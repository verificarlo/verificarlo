#!/bin/bash

rm -f test.o test output

verificarlo-c test.c -o test

export VFC_BACKENDS="libinterflop_vprec.so --mode=ib --precision-binary64=23"

./test > output

if grep "= 0x0p+0" output; then
  exit 0
else
  echo "zero + zero is not zero: test failed"
  exit 1
fi

