#!/bin/bash
set -e

output=0

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
	output=1
    fi

    remove_test_output_file
    verificarlo-c --inst-func -DREAL=$type$size -c test.c -emit-llvm --save-temps

    if grep "_"$size"x"$type$op test.*.3.ll; then
	echo "vector $op for $type$size instrumented"
    else
	echo "vector $op for $type$size NOT instrumented with --inst-func"
	output=1
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
	    if [ $output == 1 ] ; then
		result=1
	    fi
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

if [ ! $is_x86 == 0 ] ; then
    echo "You have x86 architecture"
    if [ ! $sse == 0 ] ; then
	echo "sse"	
    fi

    if [ ! $avx == 0 ] ; then
	echo "avx"
    fi

    if [ ! $avx512 == 0 ] ; then
	echo "avx512"
    fi
else
    echo "You have NOT x86 architecture"
fi

if [ $result == 0 ] ; then
    echo "TEST PASS"
else
    echo "TEST FAIL"
fi

exit $result
