#!/bin/bash
set -e

# Compile tchebychev.c using MCA lib instrumentation

# Tchebychev polynom becomes unstable around 1, when computed with
# single precision
export VERIFICARLO_PRECISION=23

verificarlo tchebychev.c -o tchebychev

# Run 15 iterations of tchebychev for all values in [.0:1.0:.01]
echo "z y" > output
for z in $(seq 0.0 0.01 1.0); do
    for i in $(seq 1 15); do
        ./tchebychev $z >> output
    done
done

# Plot the result, resulting graph is saved as Rplots.pdf
# By default this line is commented to avoid including R as a
# dependency for running the test suite.
# If you want to see the output of the run, install R and run
# the following command manually.

#./plot.R
