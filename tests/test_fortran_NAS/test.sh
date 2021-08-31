#!/bin/bash
set -e

source ../paths.sh

if [ -z "${FLANG_PATH}" ]; then
    echo "this test is not run when not using --with-flang"
    exit 0
fi

cd NPB3.0-SER
mkdir -p bin
./run-bench.sh

echo 'test successed'
exit 0
