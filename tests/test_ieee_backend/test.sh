#!/bin/sh
set -e
verificarlo -O0 test.c -o test
VFC_BACKENDS="libinterflop_ieee.so" ./test
