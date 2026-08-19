#!/bin/bash
set -e

source ../paths.sh

bash clean.sh
verificarlo-c test.c -o test
${GCC_PATH:-gcc} test_dlopen.c -o test_dlopen -ldl

export VFC_BACKENDS="libinterflop_ieee.so"
export VFC_BACKENDS_LOGGER=False

echo "Running test binary linked against libinterflop_vfcwrapper..."
./test > runtime.log 2>&1
grep -q "Result: 4.000000" runtime.log

echo "Running dlopen test binary..."
./test_dlopen >> runtime.log 2>&1

echo "Verifying no .vfcwrapper temporary object files were created during build..."
if ls .vfcwrapper* >/dev/null 2>&1; then
    echo "ERROR: temporary .vfcwrapper files found!"
    exit 1
fi

echo "All dynamic vfcwrapper architecture tests passed!"
