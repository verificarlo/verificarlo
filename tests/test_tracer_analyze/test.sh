#!/bin/bash

check_success() {
    if (( $? != 0 )); then
	echo "Test ${1} failed"
	exit 1
    fi
    echo -e "Test ${1} successed\n"
}

check_fail() {
    if (( $? == 0 )); then
	echo "Test ${1} failed"
	exit 1
    fi
    echo -e "Test ${1} successed\n"
}

compile() {
    verificarlo --tracer --tracer-level=temporary test.c -o test 
}

clean() {
    rm -rf .vtrace tmp locationInfo.map empty
}

clean
compile

veritracer analyze
check_fail 1

veritracer launch --jobs=10 --binary=./test 
veritracer analyze
check_success 2

veritracer launch --jobs=10 --binary=./test --prefix-dir=tmp
veritracer analyze --prefix-dir=tmp
check_success 3

mkdir -p empty/1 empty/2
touch empty/1/veritracer.dat
echo "2" > empty/2/veritracer.dat
veritracer analyze --prefix-dir=empty
check_fail 4

mkdir -p empty/1 empty/2
rm -f empty/2/veritracer.dat
touch empty/1/backtrace.dat
touch empty/1/veritracer.dat
touch empty/2/backtrace.dat
touch empty/2/veritracer.dat
veritracer analyze --prefix-dir=empty
check_fail 5
