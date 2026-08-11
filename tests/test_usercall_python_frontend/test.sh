#!/bin/bash

set -e

# Silence backend loading banners to keep test output clean.
export VFC_BACKENDS_SILENT_LOAD=True

check_status() {
    if [ $? -ne 0 ]; then
        echo "Test fail"
        exit 1
    fi
}

# -----------------------------------------------------------------------
# 0. Prerequisites
# -----------------------------------------------------------------------
if ! python3 -c "from verificarlo.usercall.interflop_call import set_precision_binary32" 2>/dev/null; then
    echo "verificarlo.usercall not importable -- skipping"
    exit 77
fi

# -----------------------------------------------------------------------
# 1. Build the verificarlo-instrumented shared library
# -----------------------------------------------------------------------
verificarlo-c -shared -fPIC compute.c -o libcompute.so
check_status

# -----------------------------------------------------------------------
# 2. Unit tests (no backend required)
# -----------------------------------------------------------------------
python3 test.py unit
check_status

# -----------------------------------------------------------------------
# 3. MCA backend tests
#    Fixed seed for reproducibility; MCA in default RR mode.
# -----------------------------------------------------------------------
VFC_BACKENDS="libinterflop_mca.so --seed=1234" \
    python3 test.py mca ./libcompute.so
check_status

# -----------------------------------------------------------------------
# 4. vprec backend tests
#    Full precision to start with; tests reduce the range mid-execution.
# -----------------------------------------------------------------------
VFC_BACKENDS="libinterflop_vprec.so \
    --precision-binary32=23 --range-binary32=8 \
    --precision-binary64=52 --range-binary64=11" \
    python3 test.py vprec ./libcompute.so
check_status

echo "All tests passed"
