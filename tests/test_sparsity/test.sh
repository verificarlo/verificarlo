#!/bin/bash
#set -e

Timeit() {
  START="$(date +%s%N)"
  for i in {1..100}; do
    ./test 2> /dev/null
  done
  DURATION=$[ $(date +%s%N) - ${START} ]
  echo $DURATION
}

Check() {
    for SPARSE in "1" "2" "5" "10" "20" "100"; do 
	echo -e "\nChecking backend MCA at SPARSITY ${SPARSE}"

	BACKENDSO="libinterflop_mca.so"
	export VFC_BACKENDS="${BACKENDSO} --sparsity=$SPARSE"
  Timeit

done

	echo -e "\nChecking backend IEEE backend"

	BACKENDSO="libinterflop_ieee.so"
	export VFC_BACKENDS="${BACKENDSO}"
  Timeit
}

echo "Checking"
verificarlo-c -O0  test.c -o test
Check 53

echo -e "\nsuccess"
