#!/bin/bash

echo "-O0"
gcc test.c -o test -O0
./test

echo "-O3"
gcc test.c -o test -O3
./test

echo "-Ofast"
gcc test.c -o test -Ofast
./test
