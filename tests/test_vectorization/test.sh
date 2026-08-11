#!/bin/bash

set -e

source ../paths.sh

# ${LLVM_BINDIR}/clang -Wall -Wextra  -O3 -march=native print.c operation.c test.c -g -o test-clang -I.
# ${GCC_PATH}          -Wall -Wextra  -O3 -march=native print.c operation.c test.c -g -o test-gcc -I.
# verificarlo-c        -Wall -Wextra  -O3 -march=native print.c operation.c test.c -g -o test-mca -I.

clang=${LLVM_BINDIR}/clang
gcc=${GCC_PATH}
mca=verificarlo-c

cflags="-Wall -Wextra -O3 ${MARCH_FLAG} -I. -g"

parallel --header : "{compiler} ${cflags} print.c operation.c test.c -o test-{#}" ::: compiler $gcc $clang $mca

if [[ $? != 0 ]]; then
    echo "Test failed"
    exit 1
fi

export VFC_BACKENDS_SILENT_LOAD="True"
export VFC_BACKENDS_LOGGER="False"
export VFC_BACKENDS="libinterflop_ieee.so"

./test-1 2>gcc.log
./test-2 2>clang.log
./test-3 2>mca.log

diff3 gcc.log clang.log mca.log > "diff.log"
if [[ -z $(cat diff.log) ]]; then
    echo "Test successed"
    exit 0
else
    echo "Test failed"
    cat diff.log
    exit 1
fi
