#!/bin/bash

temporary="*~"
binary="*.asm *.o bin"
instrument="wrapper ieee vprec mca"
result="result.txt output*"

rm -Rf $temporary $instrument $binary $result
