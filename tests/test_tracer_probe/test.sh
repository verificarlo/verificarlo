#!/bin/bash

function grep_variable { 
    
    if (( $( grep -q "r->x" locationInfo.map ) )); then
       echo "Error variable not found"
    fi
    if (( $( grep -q "r->y" locationInfo.map  ) )); then
	echo "Error variable not found"
    fi
    if (( $( grep -q "r->t[5]" locationInfo.map ) )); then
       echo "Error variable not found"
    fi
}

function compile() {
    rm -f locationInfo.map
    verificarlo test.c -o test --tracer --verbose ${1}
    grep_variable
}

compile -O0
compile -O1
compile -O2
compile -O3 -ffast-math

