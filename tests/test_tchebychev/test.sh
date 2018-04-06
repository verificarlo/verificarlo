#!/bin/bash
set -e

# Compile tchebychev.c using MCA lib instrumentation

# Tchebychev polynom becomes unstable around 1, when computed with
# single precision
export VERIFICARLO_PRECISION=23
export LC_ALL=C

METHOD=EXPANDED
if [ $# -eq 1 ]; then
    METHOD=$1
fi

case "${METHOD}" in
    EXPANDED) ;;
    FACTORED) ;;
    HORNER) ;; 
    *)
	echo "Inexsting method $1, choose between {EXPANDED|FACTORED|HORNER}"
	exit 1
esac
	    
verificarlo tchebychev.c -o tchebychev

# Run 15 iterations of tchebychev for all values in [.0:1.0:.01]
echo "z y" > $METHOD
# for z in $(seq 0.0 0.01 1.0); do 
for z in $(seq 0.75 0.001 1.0); do # For zooming on the interesting part 
    for i in $(seq 1 15); do
        ./tchebychev $z $METHOD >> $METHOD
    done
done

# Plot the result, resulting graph is saved as Rplots.pdf
# By default this line is commented to avoid including R as a
# dependency for running the test suite.
# If you want to see the output of the run, install R and run
# the following command manually.

# ./plot.R $METHOD
