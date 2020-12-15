#!/bin/bash

tests="$tests $(find . -maxdepth 1 -type d -name 'test_*' | sort)"

count=$(echo $tests | wc -w)

# Print number of tests detected
echo "1..$count"

for t in $tests; do
    echo $t
    bash -c -l "cd $t && ./clean.sh"
done

rm -Rf *~ testplan.log testplan.trs
