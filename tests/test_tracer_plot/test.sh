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
    verificarlo --tracer --tracer-level=temporary test.c -o test 
}

clean() {
    rm -rf .vtrace tmp locationInfo.map
}

clean
compile


veritracer launch --jobs=10 --binary=./test
veritracer analyze

veritracer plot .vtrace/veritracer.000bt --no-show 
check_success 1

veritracer plot .vtrace/veritracer.000bt --no-show --location-info-map=
check_success 2

veritracer plot .vtrace/veritracer.000bt --no-show --output=image.png
file image.png 
check_success 3

veritracer plot .vtrace/veritracer.000bt --no-show --output=image.pdf
file image.pdf
check_success 4



