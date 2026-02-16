#!/bin/bash
#
# VPREC test orchestrator.
# For each (type, mode, range, precision) combination:
#   1. Set VFC_BACKENDS with appropriate VPREC parameters
#   2. Run compute_vprec_rounding against the pre-generated reference file
#   3. Run check_output to compare results
#
set -e

export VFC_BACKENDS_LOGGER=False

modelist="ob ib full"
typeList="float double"
inc=1
runListFile=$(mktemp)
trap "rm -f ${runListFile}" EXIT

run_single_test() {
    local realType=$1
    local mode=$2
    local range=$3
    local precision=$4

    # Compute option names directly (associative arrays can't be exported to parallel)
    if [ "${realType}" = "float" ]; then
        local _range="--range-binary32=${range}"
        local _precision="--precision-binary32=${precision}"
    else
        local _range="--range-binary64=${range}"
        local _precision="--precision-binary64=${precision}"
    fi

    local _mode="--mode=${mode}"
    export VFC_BACKENDS="libinterflop_vprec.so ${_precision} ${_range} ${_mode}"

    local fileRef="./mpfr_reference/mpfr_${realType}_${mode}_E${range}M${precision}"
    local fileVprec="vprec_output_${realType}_${mode}_E${range}M${precision}.txt"

    if [ ! -f "${fileRef}" ]; then
        echo "Reference file not found: ${fileRef}"
        return 1
    fi

    # Run VPREC computation (instrumented by verificarlo)
    ./compute_vprec_rounding_${realType} "${fileRef}" >"${fileVprec}"

    # Check results (not instrumented)
    ./check_output "${fileRef}" "${fileVprec}" ${range} ${precision}
    local status=$?

    rm -f "${fileVprec}"
    return ${status}
}

export -f run_single_test

for mode in ${modelist}; do
    for realType in ${typeList}; do
        if [ "${realType}" = "float" ]; then
            rangelist=$(seq 4 ${inc} 8)
            precisionlist=$(seq 3 ${inc} 23)
        fi

        if [ "${realType}" = "double" ]; then
            rangelist=$(seq 5 ${inc} 11)
            precisionlist=$(seq 4 ${inc} 52)
        fi

        for range in ${rangelist}; do
            for precision in ${precisionlist}; do
                echo "run_single_test ${realType} ${mode} ${range} ${precision}" >>"${runListFile}"
            done
        done
    done
done

echo "Running tests in parallel..."
if command -v parallel &>/dev/null; then
    parallel --group --halt now,fail=1 -j $(nproc) :::: ${runListFile} || exit 42
else
    # Fallback to sequential execution
    while IFS= read -r cmd; do
        [ -z "$cmd" ] && continue
        eval "$cmd" || exit 42
    done <${runListFile}
fi

echo "All tests passed."
