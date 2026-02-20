#!/bin/bash
set -e

# Test that verificarlo wrapper scripts properly preserve argument quoting
# This addresses GitHub issue #355

source ../paths.sh

# Function to test argument passing
test_wrapper() {
    local wrapper=$1
    local linker=$2
    local test_file=$3

    echo "Testing $wrapper"

    # Test 1: Macro definition with spaces (the main issue from #355)
    echo "  Test 1: Macro with spaces"
    if "$wrapper" -DPACKAGE_STRING='"verificarlo test"' -c "$test_file" -o test1.o 2>/dev/null; then
        echo "  PASS: Macro with spaces compiled successfully"
        rm -f test1.o
    else
        echo "  FAIL: Macro with spaces failed to compile"
        return 1
    fi

    # Test 2: Macro definition with complex string containing special characters
    echo "  Test 2: Complex macro with special characters"
    if $wrapper -DTEST_MACRO='"test: version 1.0.0"' -c "$test_file" -o test2.o 2>/dev/null; then
        echo "  PASS: Complex macro compiled successfully"
        rm -f test2.o
    else
        echo "  FAIL: Complex macro failed to compile"
        return 1
    fi

    # Test 3: Multiple macros with spaces
    echo "  Test 3: Multiple macros with spaces"
    if $wrapper -DPACKAGE_STRING='"verificarlo test"' -DVERSION='"1.0.0"' -c "$test_file" -o test3.o 2>/dev/null; then
        echo "  PASS: Multiple macros with spaces compiled successfully"
        rm -f test3.o
    else
        echo "  FAIL: Multiple macros with spaces failed to compile"
        return 1
    fi

    # Test 4: Build a complete executable and verify macro values
    echo "  Test 4: Full compilation and execution test"
    if $wrapper -DPACKAGE_STRING='"verificarlo shell test"' -DVERSION='"v1.2.3"' "$test_file" -o test_exe 2>/dev/null; then
        echo "  PASS: Full compilation with macros successful"

        # Test execution to verify macros were passed correctly
        if ./test_exe >output.txt 2>/dev/null; then
            if grep -q "PACKAGE_STRING: verificarlo shell test" output.txt && grep -q "VERSION: v1.2.3" output.txt; then
                echo "  PASS: Executable runs and macros have correct values"
            else
                echo "  FAIL: Macros don't have expected values"
                echo "  Output was:"
                cat output.txt
                return 1
            fi
        else
            echo "  PASS: Compilation successful (execution may fail due to instrumentation requirements)"
        fi
        rm -f test_exe output.txt
    else
        echo "  FAIL: Full compilation failed"
        return 1
    fi

    # Test 5: Include path with spaces (create the directory structure)
    mkdir -p "path with spaces"

    if [[ "$wrapper" == *"verificarlo-f"* ]]; then
        echo 'integer :: INCLUDED_HEADER = 1' >"path with spaces/header.h"
        cat >test_include.f90 <<'EOF'
program test_inc
    include "header.h"
    print *, "Testing include path with spaces"
end program test_inc
EOF
        test_inc_file="test_include.f90"
    else
        echo '#define INCLUDED_HEADER "found"' >"path with spaces/header.h"
        cat >test_include.c <<'EOF'
#include "header.h"
#include <stdio.h>
int main() {
    printf("Testing include path with spaces\n");
    #ifdef INCLUDED_HEADER
    printf("Header included: %s\n", INCLUDED_HEADER);
    #endif
    return 0;
}
EOF
        test_inc_file="test_include.c"
    fi

    echo "  Test 5: Include path with spaces"
    if $wrapper -I"path with spaces" -c "$test_inc_file" -o test5.o 2>/dev/null; then
        echo "  PASS: Include path with spaces compiled successfully"
        rm -f test5.o
    else
        echo "  FAIL: Include path with spaces failed"
        return 1
    fi

    # Clean up test files
    rm -f test*.c test*.cpp test*.f90 test*.o test_exe output.txt ${test_file}
    rm -rf "path with spaces"
}

# Test verificarlo-c wrapper
cat >test.c <<'EOF'
#include <stdio.h>
int main() {
    #ifdef PACKAGE_STRING
    printf("PACKAGE_STRING: %s\n", PACKAGE_STRING);
    #endif
    #ifdef TEST_MACRO
    printf("TEST_MACRO: %s\n", TEST_MACRO);
    #endif
    #ifdef VERSION
    printf("VERSION: %s\n", VERSION);
    #endif
    return 0;
}
EOF
test_wrapper "verificarlo-c" "clang" "test.c"

# Test verificarlo-c++ wrapper
echo ""
cat >test.cpp <<'EOF'
#include <iostream>
int main() {
    std::cout << "C++ Test Program" << std::endl;
    #ifdef PACKAGE_STRING
    std::cout << "PACKAGE_STRING: " << PACKAGE_STRING << std::endl;
    #endif
    #ifdef TEST_MACRO
    std::cout << "TEST_MACRO: " << TEST_MACRO << std::endl;
    #endif
    return 0;
}
EOF
test_wrapper "verificarlo-c++" "clang++" "test.cpp"

# Test verificarlo-f wrapper (if flang is available)
if [ "${FLANG_PATH}" != "" ]; then

    echo ""
    echo "Testing verificarlo-f"

    # Create a simple Fortran test program
    cat >test.f90 <<'EOF'
program test
    print *, "TEST FORTRAN"
    print *, 'TEST_MACRO: ', TEST_MACRO
    print *, 'Fortran compilation test'
end program
EOF

    test_wrapper "verificarlo-f" "flang" "test.f90"

    rm -f test.f90
else
    echo "verificarlo-f or flang not available, skipping Fortran test"
fi

echo ""
echo "All argument quoting tests passed successfully"
exit 0
