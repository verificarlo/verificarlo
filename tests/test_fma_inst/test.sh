#!/bin/bash

# Compile the program with Verificarlo

function check_instrumentation() {

    rm -f log

    local type=$1
    verificarlo test.c -DREAL=$type -o test_${type} --save-temps --verbose --inst-fma 2>log

    # Check if the program was compiled successfully
    if [ $? -ne 0 ]; then
        echo "Compilation failed"
        exit 1
    fi

    if [ "$type" == "float" ]; then
        instruction="@llvm.(fmuladd|fma).f32"
    else
        instruction="@llvm.(fmuladd|fma).f64"
    fi

    # Check if the FMA instruction is instrumented
    if grep -qE "Instrumenting .* ${instruction}" log; then
        echo "FMA instruction is instrumented"
    else
        echo "FMA instruction is not instrumented"
        exit 1
    fi
}

# At -O3 the loop vectorizer turns the FMA into llvm.fma.v<N><type>
function check_vector_instrumentation() {

    rm -f log

    local type=$1
    verificarlo vector.c -DREAL=$type -O3 -o test_vector_${type} --save-temps --verbose --inst-fma -lm 2>log

    # Check if the program was compiled successfully
    if [ $? -ne 0 ]; then
        echo "Compilation failed"
        exit 1
    fi

    if [ "$type" == "float" ]; then
        instruction="@llvm.(fmuladd|fma).v[0-9]+f32"
    else
        instruction="@llvm.(fmuladd|fma).v[0-9]+f64"
    fi

    # Check if the vector FMA instruction is instrumented
    if grep -qE "Instrumenting .* ${instruction}" log; then
        echo "Vector FMA instruction is instrumented"
    else
        echo "Vector FMA instruction is not instrumented"
        exit 1
    fi
}

check_instrumentation "float"
check_instrumentation "double"
check_vector_instrumentation "float"
check_vector_instrumentation "double"

export VFC_BACKENDS="libinterflop_mca.so"

# Run the program multiple times and check if the output varies
function test_perturbation() {

    local type=$1

    rm -f out
    for i in {1..100}; do
        ./test_${type} 0.1 0.2 0.3 >>out
    done

    local status=$?
    if [ $status -ne 0 ]; then
        echo "Execution failed"
        exit 1
    fi

    variability=$(python3 -c "import numpy as np; print(len(set((np.loadtxt('out')))) != 1)")
    echo $variability

    if [ $variability == True ]; then
        echo "Results are different"
    else
        echo "Results are the same"
        exit 1
    fi

}

test_perturbation "float"
test_perturbation "double"
test_perturbation "vector_float"
test_perturbation "vector_double"
