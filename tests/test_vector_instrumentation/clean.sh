#!/bin/bash

temporary="*~"
binary="*.asm *.o binary_compute"
instrument="ieee vprec mca"
result="output*"

rm -Rf $temporary $instrument $binary $result
