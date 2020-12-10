#!/bin/bash
# Test the implementation of vector function in vprec backend

bin=binary_compute

# Delete past result
rm -Rf output.txt

vec="1.1 1.1"

# Compile and run the program
# Take the architecture flags on parameter and the output file
compile_and_run()
{
    export VFC_BACKENDS_SILENT_LOAD="True"
    export VFC_BACKENDS_LOGGER="False"
    
    # Run test
    touch output.txt
    export VFC_BACKENDS="libinterflop_vprec.so"
    for i in 2 4 8 16
    do
	for type in float double
	do
	    # Compile test
	    verificarlo-c -march=native -O3 -fno-slp-vectorize -DREAL=$type$i compute.c -o $bin

	    ./$bin $type "+" $i $vec >> $1
	    ./$bin $type "*" $i $vec >> $1
	    ./$bin $type "-" $i $vec >> $1
	    ./$bin $type "/" $i $vec >> $1
	done
    done

    unset VFC_BACKENDS_SILENT_LOAD
    export VFC_BACKENDS_LOGGER="True"
}

compile_and_run output.txt
is_equal=$(diff -U 0 result.txt output.txt | grep ^@ | wc -l)

# Print result
echo $is_equal

# Clean folder
rm -Rf *~ *.o $bin

# Exit
if [ $is_equal == 0 ] ; then
    exit 0;
else
    exit 1;
fi
