#!/bin/bash
set -e

verificarlo-c test.c -o test --verbose &> compile.log

if ! grep "Instrumenting" compile.log; then
  echo "New pass manager args not recognized. Test failed."
  exit 1
fi

VFC_BACKENDS="libinterflop_mca.so" ./test
