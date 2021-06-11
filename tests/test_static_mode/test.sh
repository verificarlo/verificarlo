#!/bin/sh
set -e
verificarlo-c -static -O0 test.c -o test
VFC_BACKENDS="libinterflop_ieee.so --debug" ./test
