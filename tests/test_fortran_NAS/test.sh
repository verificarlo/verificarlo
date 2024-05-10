#!/bin/bash
set -e

source ../paths.sh

if [ -z "${FLANG_PATH}" ]; then
    echo "this test is not run when not using --with-flang"
    # Exit with 77 to mark the test skipped
    # https://www.gnu.org/software/automake/manual/html_node/Scripts_002dbased-Testsuites.html
    exit 77
fi

cd NPB3.0-SER
mkdir -p bin
./run-bench.sh

echo 'test successed'
exit 0
