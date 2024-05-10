#!/bin/sh

# Mark this test as expected to fail (exit 88)
# static mode fails with
# * libinterflop_ieee.so
#   due to libquadmath not being handled correctly, it
#   fails with glibc 2.31 and works with glibc 2.35 and above
# * libinterflop_mca.so (any backends using RNG)
#   due to issues with the TLS and static libraries
#   (see https://www.akkadia.org/drepper/tls.pdf sec 3.1)

set -e
# Trap segfaults and report them as test failures
trap 'case $? in
        139) echo "Test failed"; exit 88;;
      esac' EXIT

verificarlo-c -static -O0 test.c -o test

check_status() {
    if [ $1 -ne 0 ]; then
        echo "Test failed"
        exit 88
    fi
}

echo "Running tests with ieee backend"
VFC_BACKENDS="libinterflop_ieee.so --debug" ./test
check_status $?

echo "Running tests with mca backend with ieee mode"
VFC_BACKENDS="libinterflop_mca.so --mode=ieee" ./test
check_status $?
