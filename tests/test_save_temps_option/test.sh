#!/bin/bash

set -e

find_function_in_obj() {
    nm test_${1}.o | grep loop_${1}
    if [[ $? != 0 ]]; then
        echo "Cannot find function loop_${1} in test_${1}.o"
        exit 1
    fi
}

find_function_in_ll() {
    nl=$(grep "loop_${1}(" *.ll | wc -l)
    # 4 = one declaration + one use for .1.ll and .2.ll
    if [[ $nl != 4 ]]; then
        echo "Cannot find function loop_${1} in *.ll"
        exit 1
    fi
}

check_function() {
    for i in $(seq 10); do
        find_function_in_obj $i
    done
}

check_files() {
    for i in $(seq 10); do
        find_function_in_ll $i
    done
}

new_env() {
    DIR=.$1
    rm -rf $DIR
    mkdir -p $DIR
    cp -r *.c Makefile $DIR
    cd $DIR
}

test1() {
    new_env test1
    # Check that intermediate files do not overlap
    # during sequential compilation
    make
    check_function
    make clean
}

test2() {
    new_env test2
    # Check that intermediate files do not overlap
    # during parallel compilation
    make -j
    check_function
    make clean
}

test3() {
    new_env test3
    # Check that intermediate files are kept
    # when passing option "--save-temps"
    # during sequential compilation
    make save_tmp=1
    check_files
    make clean
}

test4() {
    new_env test4
    # Check that intermediate files are kept
    # when passing option "--save-temps"
    # during parallel compilation
    make -j save_tmp=1
    check_files
    make clean
}

export -f new_env check_function check_files find_function_in_obj find_function_in_ll
export -f test1 test2 test3 test4
parallel test{#} ::: $(seq 4)

echo "pass"
exit 0
