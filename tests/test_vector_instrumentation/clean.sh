#!/bin/bash

temporary="*~"
binary="*.o run"
instrument="test.*.*.ll"

rm -Rf $temporary $instrument $binary
