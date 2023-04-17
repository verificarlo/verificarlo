#!/bin/bash
set -e

rm -f *.ll
verificarlo-c -c test.c -emit-llvm --save-temps

if grep "fcmp ogt" test.*.2.ll || grep "fcmp ole" test.*.2.ll ; then
    echo "comparison operations not instrumented"
else
    echo "comparison operations INSTRUMENTED without --inst-fcmp"
    exit 1
fi

rm -f *.ll
verificarlo-c --inst-fcmp -c test.c -emit-llvm --save-temps

if grep "fcmp ogt" test.*.2.ll || grep "fcmp ole" test.*.2.ll ; then
    echo "comparison operations NOT instrumented with --inst-fcmp"
    exit 1
else
    echo "comparison operations instrumented"
fi

# Only test vector comparisons in x86_64
if [[ $(arch) == "x86_64" ]]; then
    if grep "_4xdoublecmp" test.*.2.ll; then
        echo "vector comparison instrumented"
    else
        echo "vector comparison NOT instrumented with --inst-fcmp"
        exit 1
    fi
fi

# Test correct interposition for scalar and vector cases
verificarlo-c --inst-fcmp run.c -o run
VFC_BACKENDS="libinterflop_ieee.so --debug" ./run | sort -n 2> scalar.log

verificarlo-c --inst-fcmp -O2 run.c -o run
VFC_BACKENDS="libinterflop_ieee.so --debug" ./run | sort -n 2> vector.log

if diff scalar.log vector.log; then
    echo "Test successed"
    exit 0
else
    echo "Test failed"
    exit 1
fi
