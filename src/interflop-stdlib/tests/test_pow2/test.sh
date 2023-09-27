#!/bin/bash

echo "-O0"
gcc test.c -o test -lm -O0
./test

echo "-O3"
gcc test.c -o test -lm -O3
./test

echo "-Ofast"
gcc test.c -o test -lm -Ofast
./test
