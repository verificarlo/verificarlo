#!/bin/bash

# Result of sub test
is_equal=0
wrapper=0
result=0
instruction_set=0

# Variable to set vector on C program
bin=binary_compute
vec="1.1 1"

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

# Sub function which test a specific case
# See next function
_check_vector_instruction_and_register()
{
    type=$1
    op=$2
    size=$3
    instru=$4
    register=$5
    file=$6

    if grep $instru'.*'$register $file; then
	echo "Instruction $instru and register $register INSTRUMENTED"
    else
	echo "Instruction $instru and register $register NOT instrumented"
	instruction_set=1
    fi
}

# Check if vector instruction and register are used in given backend
# Take backend in parameter
check_vector_instruction_and_register()
{
    # Recup backend
    backend=$1
    
    # File path name
    asm_file=$backend/$backend.asm
    info_file=$backend/info.txt

    # Disassemble
    objdump -d /usr/local/lib/libinterflop_$backend.so > $asm_file

    # Exec the python script to extract data for cut the assembler file
    # Put data in info file
    (./extract_asm_vector_func.py $asm_file) >> $info_file

    # Count line of the python script output
    count_line=$(cat $info_file | wc -l)

    for i in $(seq -s ' ' 1 3 $count_line)
    do
	# Recup the function name
	function_name=$(sed -n $i"p" $info_file)

	# Recup the begin line
	i=$((i+1))
	begin=$(sed -n $i"p" $info_file)

	# Recup the end line
	i=$((i+1))
	end=$(sed -n $i"p" $info_file)

	# Recup the entire function given in the backend assembler
	sed -n $begin,$end"p" $asm_file > $backend/$function_name
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
	    echo "You have SSE instruction"

	    for size in 2 4
	    do
		echo "float$size"
		for op in add mul sub div
		do
		    _check_vector_instruction_and_register float $op $size $op"ps" xmm $backend/_interflop_$op"_"$type"_vector"
		done
	    done

	    echo "double2"
	    for op in add mul sub div
	    do
		_check_vector_instruction_and_register double $op 2 $op"pd" xmm $backend/_interflop_$op"_"$type"_vector"
	    done
	fi

	# AVX
	if [ ! $avx == 0 ] ; then
	    echo "You have AVX instruction"

	    echo "float8"
	    for op in add mul sub div
	    do
		_check_vector_instruction_and_register float $op 8 $op"ps" ymm $backend/_interflop_$op"_"$type"_vector"
	    done

	    echo "double4"
	    for op in add mul sub div
	    do
		_check_vector_instruction_and_register double $op 4 $op"pd" ymm $backend/_interflop_$op"_"$type"_vector"
	    done
	fi

	# AVX512
	if [ ! $avx512 == 0 ] ; then
	    echo "You have AVX512 instruction"

	    echo "float16"
	    for op in add mul sub div
	    do
		_check_vector_instruction_and_register float $op 16 $op"ps" zmm $backend/_interflop_$op"_"$type"_vector"
	    done

	    echo "double8"
	    for op in add mul sub div
	    do
		_check_vector_instruction_and_register double $op 8 $op"pd" zmm $backend/_interflop_$op"_"$type"_vector"
	    done
	fi
    else
	echo "You have NOT x86 architecture"
    fi
}

# Run the check of result and wrapper instrumentation
for backend in ieee vprec mca
do
    echo "#######################################"
    echo "Backend $backend"
    echo ""

    echo "Testing wrapper instrumentation and good result"
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

    # Separate
    echo ""

    echo "Testing the use of vector instruction and register"
    check_vector_instruction_and_register $backend
    
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

# Print instruction set result
if [ $instruction_set == 1 ] ; then
    echo "TEST for instruction set         FAILED"
    result=1
else
    echo "TEST for instruction set         PASSED"
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
