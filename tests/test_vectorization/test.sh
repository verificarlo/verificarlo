#!/bin/bash

source ../paths.sh

make clean
make -e LLVM_BINDIR=${LLVM_BINDIR} GCC_PATH=${GCC_PATH}

if [[ $? != 0 ]]; then
    echo "Test failed"
    exit 1
fi

export VFC_BACKENDS_SILENT_LOAD="True"
export VFC_BACKENDS_LOGGER="False"
export VFC_BACKENDS="libinterflop_ieee.so"

./test-gcc 2> gcc.log
./test-clang 2> clang.log
./test-mca 2> mca.log

diff3 gcc.log clang.log mca.log > diff
if [[ -z $diff ]]  ; then
    echo "Test successed"
    exit 0
else
    echo "Test failed"
    cat diff
    exit 1
fi
