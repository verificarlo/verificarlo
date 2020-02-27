#!/bin/sh
set -e
verificarlo-c -O0 test.c -o test

# Check that all the operations have been instrumented
for op in fadd fsub fmul fdiv; do
  if grep $op test.2.ll; then
    echo "Some $op have not been instrumented"
    exit 1
  fi
done

VFC_BACKENDS="libinterflop_ieee.so" ./test

#Test options
echo "Test interflop_ieee.so options"
VFC_BACKENDS="libinterflop_ieee.so --help" ./test

./test_options.sh
