#!/bin/bash
set -e
verificarlo-c --ddebug -g -O0 test.c -o test

# Generation run
VFC_BACKENDS="libinterflop_ieee.so --debug" VFC_DDEBUG_GEN="operations.txt" ./test
noperations=$(wc -l < operations.txt)

# Test that exclusion works in filter mode
# Exclude the first two operations (in the operations file; does not match the execution order)
NOP=2
cat operations.txt | head -n $NOP > exclusion.txt
VFC_BACKENDS="libinterflop_ieee.so --debug" VFC_DDEBUG_EXCLUDE="exclusion.txt" ./test 2> out
cat out
executed_operations=$(cat out |grep ' -> '|wc -l)
expected_operations=`expr $noperations - $NOP`
if [ $executed_operations == $expected_operations ]; then
  echo "exclusion in filter mode ok"
else
  echo "problem with exclusion, expected $expected_operations operations only after filtering"
  exit 1
fi

# Test that exclusion works in generation mode
VFC_BACKENDS="libinterflop_ieee.so --debug" VFC_DDEBUG_EXCLUDE="exclusion.txt" VFC_DDEBUG_GEN="operations2.txt" ./test

noperations2=$(wc -l < operations2.txt)
if [ $noperations2 == $expected_operations ]; then
  echo "exclusion in generate mode ok"
else
  echo "problem with exclusion, expected $expected_operations operations only after generation"
  exit 1
fi
