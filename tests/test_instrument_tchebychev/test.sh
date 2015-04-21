#!/bin/bash
set -e

# Compile tchebychev.c using MCA lib instrumentation

../../verificarlo tchebychev.c -o tchebychev

# Run 30 iterations of tchebychev for all values in [.0:1.0:.01]
echo "z y" > output
for z in $(seq 0.0 0.01 1.0); do
    for i in $(seq 1 30); do
        ./tchebychev $z >> output
    done
done

# Plot the result, resulting graph is saved as Rplots.pdf
./plot.R
