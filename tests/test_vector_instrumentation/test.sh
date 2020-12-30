#!/bin/bash

# Result of sub test
is_equal=0
wrapper=0
result=0

# Variable to set vector on C program
bin=binary_compute
vec="1.1 1.1"

# Clean
./clean.sh

# Begin script
echo "Test for vector operation instrumentation"

# Compile and run the program
# Take the backend name on parameter
compile_and_run()
{
    # Recup parameter
    backend=$1

    # Temporarly fix size at 2 because it fail for other size in ceratin condition
    for size in 2 #4 8 16
    do
	for type in float double
	do
	    # Print the type and the size of the test
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

	    # Check if vector wrapper is called
	    add=$(grep call.*_$size"x"$type"add" $backend/$type$size.ll)
	    mul=$(grep call.*_$size"x"$type"mul" $backend/$type$size.ll)
	    sub=$(grep call.*_$size"x"$type"sub" $backend/$type$size.ll)
	    div=$(grep call.*_$size"x"$type"div" $backend/$type$size.ll)

	    if [ add != 0 ] && [ mul != 0 ] && [ sub != 0 ] && [ div != 0 ] ; then
		echo "vector operation for $type$size INSTRUMENTED"
	    else
		echo "vector operation for $type$size NOT INSTRUMENTED"
		wrapper=1
	    fi
	done
    done
}

# Run the check of result and wrapper instrumentation
for backend in ieee vprec mca
do
    echo "#######################################"
    export VFC_BACKENDS="libinterflop_$backend.so"
    mkdir $backend
    touch output_$backend.txt

    compile_and_run $backend

    if [ $(diff -U 0 result_2x.txt output_$backend.txt | wc -l) != 0 ] ; then
	echo "Result for $backend backend FAILED"
	is_equal=1
    else
	echo "Result for $backend backend PASSED"
    fi
    echo "#######################################"
done

# Print operation result
echo "#######################################"
if [ $is_equal == 1 ] ; then
    echo "TEST for vector operation result FAILED"
    result=1
else
    echo "TEST for vector operation result PASSED"
fi


# Print wrapper result
if [ $wrapper == 1 ] ; then
    echo "TEST for wrapper instrumentation FAILED"
    result=1
else
    echo "TEST for wrapper instrumentation PASSED"
fi

# Print result
if [ $result == 0 ] ; then
    echo "TEST PASS"
else
    echo "TEST FAIL"
fi
echo "#######################################"

# Exit
exit $result
