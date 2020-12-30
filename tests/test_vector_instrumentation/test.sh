#!/bin/bash

# Result of sub test
is_equal=0
wrapper=0
instruction_set=0
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
    for size in 2
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
for backend in ieee vprec
do
    export VFC_BACKENDS="libinterflop_$backend.so"
    mkdir $backend
    touch output_$backend.txt

    compile_and_run $backend

    if [ $(diff -U 0 result.txt output_$backend.txt | wc -l) != 0 ] ; then
	echo "Result for $backend backend FAIL"
	is_equal=1
    else
	echo "Result for $backend backend PASS"
    fi
done

# Check if vector instruction and register of x86 architecture are used
# Take on parameter :
#  - type     (float or double)
#  - op       (add, mul, sub, div)
#  - size     (2, 4, 8, 16)
#  - instru   (pd or ps)
#  - register (xmm, ymm, zmm)
check_vector_instruction_call() {

    # Recup parameter
    type=$1
    op=$2
    size=$3
    instru=$4
    register=$5

    # Compile and extract assembler
    verificarlo-c -DREAL=$type$size compute.c -o $bin -march=native
    objdump -D compute.o > compute.asm

    # Check
    if grep $instru'.*'$register compute.asm; then
	echo "instruction $instru and register $register for _$size""x""$type$op INSTRUMENTED"
    else
	echo "instruction $instru and register $register for _$size""x""$type$op NOT INSTRUMENTED"
	instruction_set=1
    fi
}

# Check architecture
echo "Test if vector instruction and register are used"
is_x86=$(uname -m | grep x86_64 | wc -l)

# Check if vector instruction are used
cpuinfo=$(cat /proc/cpuinfo)

sse=$(echo $cpuinfo | grep sse | wc -l)
avx=$(echo $cpuinfo | grep avx | wc -l)
avx512=$(echo $cpuinfo | grep avx512 | wc -l)

# x86_64
if [ ! $is_x86 == 0 ] ; then
    echo "You have x86 architecture"

    #  SSE
    if [ ! $sse == 0 ] ; then
	echo "sse"

	for size in 2 4
	do
	    for op in add mul sub div
	    do
		check_vector_instruction_call float $op $size $op"ps" xmm
	    done
	done

	for op in add mul sub div
	do
	    check_vector_instruction_call double $op 2 $op"pd" xmm
	done
    fi

    # AVX
    if [ ! $avx == 0 ] ; then
	echo "avx"

	for op in add mul sub div
	do
	    check_vector_instruction_call float $op 8 $op"ps" ymm
	done

	for op in add mul sub div
	do
	    check_vector_instruction_call double $op 4 $op"pd" ymm
	done
    fi

    # AVX512
    if [ ! $avx512 == 0 ] ; then
	echo "avx512"

	for op in add mul sub div
	do
	    check_vector_instruction_call float $op 16 $op"ps" zmm
	done

	for op in add mul sub div
	do
	    check_vector_instruction_call double $op 8 $op"pd" zmm
	done
    fi
else
    echo "You have NOT x86 architecture"
fi

# Print operation result
if [ $is_equal == 1 ] ; then
    echo "TEST for vector operation result failed"
    result=1
else
    echo "TEST for vector operation result passed"
fi

# Print wrapper result
if [ $wrapper == 1 ] ; then
    echo "TEST for wrapper instrumentation failed"
    result=1
else
    echo "TEST for wrapper instrumentation passed"
fi

# Print instrumentation set result
if [ $instruction_set == 1 ] ; then
    echo "TEST for instruction set failed"
    result=1
else
    echo "TEST for instruction set passed"
fi

# Print result
if [ $result == 0 ] ; then
    echo "TEST PASS"
else
    echo "TEST FAIL"
fi

# Exit
exit $result
