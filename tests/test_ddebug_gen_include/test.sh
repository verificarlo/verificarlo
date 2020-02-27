#!/bin/bash
set -e
verificarlo-c --ddebug -g -O0 test.c -o test

VFC_BACKENDS="libinterflop_ieee.so --debug" VFC_DDEBUG_GEN="inclusion.txt" ./test

# Keep only first four operations (in the inclusion file; does not match the execution order)
NOP=4
cat inclusion.txt | head -n $NOP > filtered.txt

VFC_BACKENDS="libinterflop_ieee.so --debug" VFC_DDEBUG_INCLUDE="filtered.txt" ./test 2> out
cat out
executed_operations=$(cat out |grep ' -> '|wc -l)

if [ $executed_operations == $NOP ]; then
  echo "generation and filtering ok"
else
  echo "problem with generation and filtering, expected $NOP operations only after filtering"
  exit 1
fi


