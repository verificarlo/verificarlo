#!/bin/bash

temporary="*~"
binary="*.asm *.o run"
instrument="wrapper_log"

rm -Rf $temporary $instrument $binary
