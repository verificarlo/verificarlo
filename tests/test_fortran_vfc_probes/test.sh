#!/bin/bash
set -e

source ../paths.sh

if [ -z "${FLANG}" ]; then
    echo "this test is not run when not using --with-flang"
    exit 0
fi

verificarlo-f -c vfc_probes_test.f90 --show-cmd
verificarlo-f vfc_probes_test.o -lvfc_probes -lvfc_probes_f -o test_fortran

VFC_BACKENDS="libinterflop_ieee.so" VFC_PROBES_OUTPUT="tmp.csv" ./test_fortran

if ls *.csv; then
    echo "CSV file found, SUCCESS"
    exit 0
else
    echo "CSV file not found, FAILURE"
    exit 1
fi
