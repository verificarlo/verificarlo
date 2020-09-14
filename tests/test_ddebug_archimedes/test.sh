#!/bin/bash

make clean
make dd

if grep "archimedes.c:16" dd.line/rddmin-cmp/dd.line.exclude ; then
  if grep "archimedes.c:17" dd.line/rddmin-cmp/dd.line.exclude ; then
    echo "success !"
    exit 0
  else
    echo "missing line 17 (cancellation)"
    exit 1
  fi
else
  echo "missing line 16 (round-off)"
  exit 1
fi
