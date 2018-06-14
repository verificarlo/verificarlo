#!/bin/bash

check_success() {
    if (( $? != 0 )); then
	echo "Test ${1} failed"
	exit 1
    fi
    echo "Test ${1} successed"
}

check_fail() {
    if (( $? == 0 )); then
	echo "Test ${1} failed"
	exit 1
    fi
    echo "Test ${1} successed"
}

compile() {
    verificarlo --tracer test.c -o test 
}

clean() {
    rm -rf .vtrace tmp locationInfo.map
}

clean
compile


veritracer launch --jobs=1 --binary=./test
check_success 1

veritracer launch --jobs=0 --binary=./test
check_fail 2

veritracer launch --jobs=2 --binary=./test --prefix-dir=tmp
check_success 3

veritracer launch --jobs=2 --binary=./test --prefix-dir=tmp/tmp
check_fail 4

veritracer launch --jobs=2 --binary=./test --prefix-dir=tmp/tmp --force
check_success 5

veritracer launch --jobs=2 --binary=$PWD/test --prefix-dir=tmp/tmp --force
check_success 6

veritracer launch --jobs=2 --binary=test --prefix-dir=/tmp/ --force
check_success 7



