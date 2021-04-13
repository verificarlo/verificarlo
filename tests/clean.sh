#!/bin/bash

tests="$tests $(find . -maxdepth 1 -type d -name 'test_*' | sort)"

count=$(echo $tests | wc -w)

# Print number of tests detected
echo "1..$count tests"

for t in $tests ; do
    if [ -e $t/clean.sh ] ; then
        echo $t OK
        bash -c -l "cd $t && ./clean.sh"
    else
        echo $t does not have a clean.sh script
    fi
done

rm -Rf *~ paths.sh testplan.log testplan.trs test-suite.log
