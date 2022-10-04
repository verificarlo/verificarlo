#!/bin/bash
set -e
# extract x[0] samples for plotting test results
echo "X0" > x0.dat
cat output |grep e+ |cut -d' ' -f1 >> x0.dat
./plot.R
