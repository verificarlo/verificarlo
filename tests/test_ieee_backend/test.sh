#!/bin/sh
set -e
verificarlo -O0 test.c -o test
VFC_BACKENDS="/usr/local/lib/libinterflop_ieee.so" ./test
