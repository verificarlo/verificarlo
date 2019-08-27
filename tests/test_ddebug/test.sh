#!/bin/bash
# set -e

make dd
cat dd.line/ref/*
if grep "integrate.hxx:23" dd.line/rddmin-cmp/dd.line.exclude; then
  echo "found integrate.hxx:23 in exclude rddmin"
else
  echo "MISSING integrate.hxx:23 in exclude rddmin"
  exit 1
fi
