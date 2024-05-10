#!/bin/bash
set -e

# required so that seq behaves correctly in all locales
export LC_ALL=C

# Compile tchebychev.c using MCA lib instrumentation

# Tchebychev polynom becomes unstable around 1, when computed with
# single precision
export VFC_BACKENDS="libinterflop_mca.so --precision-binary32 23"
export VFC_BACKENDS_SILENT_LOAD="True"
export VFC_BACKENDS_LOGGER="False"

verificarlo-c tchebychev.c -o tchebychev

echo "z y" >output
# Run 15 iterations of tchebychev for all values in [.0:1.0:.01]
# Parallelize the execution by chunks of 25
parallel -k -j $(nproc) -n 25 --header : "for i in {1..15} ; do ./tchebychev {z} ; done" ::: z $(seq 0.0 0.01 1.0) >>output

# Plot the result, resulting graph is saved as Rplots.pdf
# By default this line is commented to avoid including R as a
# dependency for running the test suite.
# If you want to see the output of the run, install R and run
# the following command manually.

#./plot.R
