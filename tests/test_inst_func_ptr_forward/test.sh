#!/bin/bash

# Regression test for pointer argument forwarding under --inst-func.
# When a function takes a pointer argument and passes it to another function,
# getSizeOf used to enter infinite recursion.

set -eo pipefail

export VFC_BACKENDS_LOGGER=False
./clean.sh

# Bounded compilation: if the compiler hangs due to infinite recursion,
# timeout will kill it after 15 seconds.
timeout 15 verificarlo-c -O2 test.c -o test --inst-func -lm

VFC_BACKENDS="libinterflop_ieee.so" ./test > out.txt
grep -q "^2.0$" out.txt

echo "test passed"
exit 0
