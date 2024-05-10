#!/bin/bash

export VFC_BACKENDS="libinterflop_mca.so;"
if python3 --version 2>/dev/null; then
  if ! python3 -c "import cython" 2>/dev/null; then
    echo "this test is not running without Cython installed"
    # Exit with 77 to mark the test skipped
    # https://www.gnu.org/software/automake/manual/html_node/Scripts_002dbased-Testsuites.html
    exit 77
  fi
else
  echo "this test is not running without python3 installed"
  # Exit with 77 to mark the test skipped
  # https://www.gnu.org/software/automake/manual/html_node/Scripts_002dbased-Testsuites.html
  exit 77
fi

str="verificarlo-c"

export CC=$str
export FC=$str
export LDSHARED="$str -shared"

python3 setup.py clean
# Build library
rm -f *.so
python3 setup.py clean
python3 setup.py build_ext --inplace

num=200
# Test build
python3 -c "from test import test_function as f; f($num)"
