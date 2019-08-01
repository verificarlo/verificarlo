#!/bin/bash

str="verificarlo"

export CC=$str
export FC=$str
export LDSHARED="$str -shared"

# Build library
python3 setup.py build_ext --inplace

num=200
# Test build
python3 -c "from test import test_function as f; f($num)"
