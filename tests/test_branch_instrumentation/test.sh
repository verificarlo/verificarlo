#!/bin/bash
set -e

verificarlo -c test.c

if grep "fcmp ogt" test.2.ll || grep "fcmp ole" test.2.ll ; then
  echo "comparison operations NOT instrumented"
  exit 1
else
  echo "comparison operations instrumented"
fi

if grep "_4xdoublecmp" test.2.ll; then
  echo "vector comparison instrumented"
else
  echo "vector comparison NOT instrumented"
  exit 1
fi

exit 0
