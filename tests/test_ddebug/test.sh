#!/bin/bash
set -e

make dd
if grep "integrate.hxx:23" dd.line/rddmin-cmp/dd.line.exclude; then
  echo "found integrate.hxx:23 in exclude rddmin"
else
  echo "MISSING integrate.hxx:23 in exclude rddmin"
  exit 1
fi
