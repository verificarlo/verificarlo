#!/bin/bash

temporary="*~"
binary="*.asm *.o run"
instrument="test.*.*.ll"

rm -Rf $temporary $instrument $binary
