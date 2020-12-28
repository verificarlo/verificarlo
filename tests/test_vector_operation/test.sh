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
    backend=$1
    touch output_$backend.txt
    export VFC_BACKENDS="libinterflop_$backend.so"
    mkdir $backend

    for size in 2 4 8 16
    do
	for type in float double
	do
	    #
	    echo $type$size
	    
	    # Compile test
	    verificarlo-c -march=native -DREAL=$type$size compute.c -o $bin --save-temps
	    rm *.1.ll
	    mv *.2.ll $backend/$type$size.ll

	    # Run test
	    ./$bin $type "+" $size $vec >> output_$backend.txt
	    ./$bin $type "*" $size $vec >> output_$backend.txt
	    ./$bin $type "-" $size $vec >> output_$backend.txt
	    ./$bin $type "/" $size $vec >> output_$backend.txt
	done
    done
}

is_equal=0

for backend in ieee vprec mca
do
    compile_and_run $backend
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
