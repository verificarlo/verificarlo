#!/bin/bash
set -e

rm -f *.2.ll
verificarlo-c -c test.c -emit-llvm --save-temps

if grep "fcmp ogt" test.*.2.ll || grep "fcmp ole" test.*.2.ll ; then
  echo "comparison operations not instrumented"
else
  echo "comparison operations INSTRUMENTED without --inst-fcmp"
  exit 1
fi

rm -f *.2.ll
verificarlo-c --inst-fcmp -c test.c -emit-llvm --save-temps

if grep "fcmp ogt" test.*.2.ll || grep "fcmp ole" test.*.2.ll ; then
  echo "comparison operations NOT instrumented with --inst-fcmp"
  exit 1
else
  echo "comparison operations instrumented"
fi

if grep "_4xdoublecmp" test.*.2.ll; then
  echo "vector comparison instrumented"
else
  echo "vector comparison NOT instrumented with --inst-fcmp"
  exit 1
fi

# Test correct interposition for scalar and vector cases
verificarlo-c --inst-fcmp run.c -o run
VFC_BACKENDS="libinterflop_ieee.so --debug" ./run
verificarlo-c --inst-fcmp -O2 run.c -o run
VFC_BACKENDS="libinterflop_ieee.so --debug" ./run

exit 0
