#!/bin/bash
set -e

wrapper=0
instruction_set=0

# Function
remove_test_output_file() {
    rm -Rf test.*.*.ll
}

check_wrapper_call() {
    type=$1
    op=$2
    size=$3
    
    remove_test_output_file
    verificarlo-c -DREAL=$type$size -c test.c -emit-llvm --save-temps

    if grep "_"$size"x"$type$op test.*.2.ll; then
	echo "vector $op for $type$size INSTRUMENTED without --inst-func"
    else
	echo "vector $op for $type$size not instrumented"
	wrapper=1
    fi

    remove_test_output_file
    verificarlo-c --inst-func -DREAL=$type$size -c test.c -emit-llvm --save-temps

    if grep "_"$size"x"$type$op test.*.3.ll; then
	echo "vector $op for $type$size instrumented"
    else
	echo "vector $op for $type$size NOT instrumented with --inst-func"
	wrapper=1
    fi
}

check_vector_instruction_call() {
    type=$1
    op=$2
    size=$3
    instru=$4
    register=$5

    rm -Rf test.o test.asm
    clang -DREAL=$type$size -c test.c -o test.o -march=native -fno-slp-vectorize
    objdump -d test.o > test.asm

    if grep $instru'.*'$register test.asm; then
	echo "instruction $instru and register $register instrumented"
    else
	echo "instruction $instru and register $register NOT instrumented"
	instruction_set=1
    fi
}


# Begin script
echo "Test for vector operation instrumentation"

result=0

for size in 2 4 8 16
do
    for type in float double
    do
	for op in add mul sub div
	do
	    check_wrapper_call $type $op $size
	done
    done
done

# Check architecture
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
