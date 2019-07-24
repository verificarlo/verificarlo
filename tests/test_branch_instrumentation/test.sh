#!/bin/bash
set -e

verificarlo -c test.c

if grep "fcmp ogt" test.2.ll || grep "fcmp ole" test.2.ll ; then
  echo "comparison operations were not instrumented"
  exit 1
else
  echo "comparison operations were instrumented"
  exit 0
fi

