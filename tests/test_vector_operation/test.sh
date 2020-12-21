#!/bin/bash
# Test the implementation of vector function in vprec backend

bin=binary_compute

# Delete past result
./clean.sh

vec="1.1 1.1"

# Compile and run the program
# Take the architecture flags on parameter and the output file
compile_and_run()
{
    touch $1
    export VFC_BACKENDS="$2"

    for i in 2 4 8 16
    do
	for type in float double
	do
	    # Compile test
	    verificarlo-c -march=native -O3 -fno-slp-vectorize -DREAL=$type$i compute.c -o $bin

	    # Run test
	    ./$bin $type "+" $i $vec >> $1
	    ./$bin $type "*" $i $vec >> $1
	    ./$bin $type "-" $i $vec >> $1
	    ./$bin $type "/" $i $vec >> $1
	done
    done
}

is_equal=0

for backend in ieee vprec mca
do
    compile_and_run output_$backend.txt libinterflop_$backend.so
    if [ $(diff -U 0 result.txt output_$backend.txt | grep ^@ | wc -l) != 0 ] ; then
	is_equal=1
    fi
done

# Print result
echo $is_equal

# Exit
if [ $is_equal == 0 ] ; then
    exit 0;
else
    exit 1;
fi
