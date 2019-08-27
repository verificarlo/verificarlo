#!/bin/bash
# set -e

make dd

echo "=============================================================="
echo "Ok instructions are:"
cat dd.line/rddmin-cmp/dd.line.include
echo "=============================================================="
echo "Problematic instructions are:"
cat dd.line/rddmin-cmp/dd.line.exclude
echo "=============================================================="

if grep "integrate.hxx:23" dd.line/rddmin-cmp/dd.line.exclude > /dev/null ; then
  echo "found integrate.hxx:23 in exclude rddmin"
else
  echo "MISSING integrate.hxx:23 in exclude rddmin"
  cat dd.line/ref/*
  exit 1
fi
