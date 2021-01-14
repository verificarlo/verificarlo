#!/bin/bash

# Clean
./clean.sh

# Result of sub test
is_equal=0
wrapper=0
result=0
instruction_set=0

# Variable for C program
bin=binary_compute
vec="1.1 1"

# Variable of test
list_of_backend="ieee vprec mca"
list_of_precision="float double"
list_of_op="add mul sub div"
list_of_size="2 4 8 16"

# Print test parameter
echo "backend: $list_of_backend"
echo "precision: $list_of_precision"
echo "operation: $list_of_op"
echo "size: $list_of_size"

# Separate output
echo ""

# Check architecture
is_x86=$(uname -m | grep x86_64 | wc -l)

# Check if vector instruction are used
cpuinfo=$(cat /proc/cpuinfo)
sse=$(echo $cpuinfo | grep sse | wc -l)
avx=$(echo $cpuinfo | grep avx | wc -l)
avx512=$(echo $cpuinfo | grep avx512 | wc -l)

# Quit if it is not a x86_64 cpu
if [ $is_x86 == 0 ] ; then
    echo "You have NOT x86 architecture"
    exit 1
fi
echo "You have x86 architecture"

# Generate result file
print_result() {
    
    for op in $list_of_op ; do

	# Print precision op size
	if [ $op == add ] ; then
	    echo "$1 + $2" >> result.txt 
	elif [ $op == mul ] ; then
	    echo "$1 * $2" >> result.txt 
	elif [ $op == sub ] ; then
	    echo "$1 - $2" >> result.txt 
	else
	    echo "$1 / $2" >> result.txt 
	fi

	# Print result
	for i in $(seq 1 $2) ; do
	    if [ $op == add ] ; then
		echo "2.100000" >> result.txt 
	    elif [ $op == mul ] ; then
		echo "1.100000" >> result.txt 
	    elif [ $op == sub ] ; then
		echo "0.100000" >> result.txt 
	    else
		echo "1.100000" >> result.txt 
	    fi
	done
    done
}

# SSE
if [ ! $sse == 0 ] ; then
    print_result float 2
    print_result double 2
    print_result float 4
fi

# AVX
if [ ! $avx == 0 ] ; then
    print_result double 4
    print_result float 8
fi

# AVX512
if [ ! $avx512 == 0 ] ; then
    print_result double 8
    print_result float 16
fi

# Compile for all size and all precision
mkdir bin wrapper
echo -n "Compiling all tests..."
for size in 2 4 8 16 ; do
    for precision in $list_of_precision ; do
	# Compile test
	verificarlo-c -march=native -DREAL=$precision$size compute.c -o bin/$bin"_"$precision$size --save-temps
	rm *.1.ll
	mv *.2.ll wrapper/$precision$size.ll
    done
done
echo "DONE"

# Begin script
echo "Test for vector operation instrumentation"

# Run the program and check the result
# Take the backend name, precision and size in parameter
check_result() {

    # Recup parameter
    backend=$1
    precision=$2
    size=$3

    # Print test
    echo "$precision$size"

    # Run test
    ./bin/$bin"_"$precision$size $precision "+" $size $vec >> output_$backend.txt
    ./bin/$bin"_"$precision$size $precision "*" $size $vec >> output_$backend.txt
    ./bin/$bin"_"$precision$size $precision "-" $size $vec >> output_$backend.txt
    ./bin/$bin"_"$precision$size $precision "/" $size $vec >> output_$backend.txt


    if [ $(diff -U 0 result.txt output_$backend.txt | wc -l) != 0 ] ; then
	echo "Result for $backend backend FAILED"
	is_equal=1
    else
	echo "Result for $backend backend PASSED"
    fi
}

# Check if vector wrapper are called for all size and precision
# Take backend name in parameter
check_wrapper() {

    # Recup parameter
    backend=$1

    for size in $list_of_size ; do
	for precision in $list_of_precision ; do
	    # Print test
	    echo "$precision$size"

	    # Check if vector wrapper are called
	    add=$(grep call.*_$size"x"$precision"add" wrapper/$precision$size.ll)
	    mul=$(grep call.*_$size"x"$precision"mul" wrapper/$precision$size.ll)
	    sub=$(grep call.*_$size"x"$precision"sub" wrapper/$precision$size.ll)
	    div=$(grep call.*_$size"x"$precision"div" wrapper/$precision$size.ll)

	    # Print and set result of test
	    if [ add != 0 ] && [ mul != 0 ] && [ sub != 0 ] && [ div != 0 ] ; then
		echo "vector operation for $precision$size INSTRUMENTED"
	    else
		echo "vector operation for $precision$size NOT INSTRUMENTED"
		wrapper=1
	    fi
	done
    done
}

# Sub function which test a specific case
# See next function
_check_vector_instruction_and_register() {

    # Recup parameter
    precision=$1
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

    # Disassemble the backend assembler
    objdump -d /usr/local/lib/libinterflop_$backend.so > $asm_file

    # Exec the python script to extract the begining end the end lines of all vector functions
    # Put data in info file
    (./extract_asm_vector_func.py $asm_file) >> $info_file

    # Count line of the python script output
    count_line=$(cat $info_file | wc -l)

    # For each vector function we have 3 lines
    # 1. name of the function
    # 2. begining line
    # 3. ending line
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

    #  SSE
    if [ ! $sse == 0 ] ; then
	echo "You have SSE instruction"

	for size in 2 4
	do
	    echo "float$size"
	    for op in $list_of_op
	    do
		_check_vector_instruction_and_register float $op $size $op"ps" xmm $backend/_interflop_$op"_"$precision"_vector"
	    done
	done

	echo "double2"
	for op in $list_of_op
	do
	    _check_vector_instruction_and_register double $op 2 $op"pd" xmm $backend/_interflop_$op"_"$precision"_vector"
	done
    fi

    # AVX
    if [ ! $avx == 0 ] ; then
	echo "You have AVX instruction"

	echo "float8"
	for op in $list_of_op
	do
	    _check_vector_instruction_and_register float $op 8 $op"ps" ymm $backend/_interflop_$op"_"$precision"_vector"
	done

	echo "double4"
	for op in $list_of_op
	do
	    _check_vector_instruction_and_register double $op 4 $op"pd" ymm $backend/_interflop_$op"_"$precision"_vector"
	done
    fi

    # AVX512
    if [ ! $avx512 == 0 ] ; then
	echo "You have AVX512 instruction"

	echo "float16"
	for op in $list_of_op
	do
	    _check_vector_instruction_and_register float $op 16 $op"ps" zmm $backend/_interflop_$op"_"$precision"_vector"
	done

	echo "double8"
	for op in $list_of_op
	do
	    _check_vector_instruction_and_register double $op 8 $op"pd" zmm $backend/_interflop_$op"_"$precision"_vector"
	done
    fi
}

# Run the check of result and wrapper instrumentation
for backend in $list_of_backend
do
    echo "#######################################"
    echo "Backend $backend"
    echo ""

    export VFC_BACKENDS="libinterflop_$backend.so"
    mkdir $backend
    touch output_$backend.txt

    echo "Testing good result"

    # SSE
    if [ ! $sse == 0 ] ; then
	check_result $backend float 2
	check_result $backend double 2
	check_result $backend float 4
    fi

    # AVX
    if [ ! $avx == 0 ] ; then
	check_result $backend double 4
	check_result $backend float 8
    fi

    # AVX512
    if [ ! $avx512 == 0 ] ; then
	check_result $backend double 8
	check_result $backend float 16
    fi

    # Separate output
    echo ""

    echo "Testing wrapper instrumentation"

    check_wrapper $backend
    
    # Separate output
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
