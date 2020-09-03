#!/bin/bash
set -e


source ../paths.sh

# Clean up previous coverage files
rm -f *.gcno *.gcda *.gcov

# To use verificarlo-c with branch coverage we add the --coverage cli option
verificarlo-c --coverage unst_branch.c -o unst_branch --save-temps

gcov_report() {
  # Clean up previous coverage files
  rm -f *.gcda *.gcov

  # Run the program 50 times
  for i in $(seq 50); do
    ./unst_branch > /dev/null
  done

  # The llvm-cov tool generates a gcov branch report
  $LLVM_BINDIR/llvm-cov gcov -f -b unst_branch.*.1.gcda > /dev/null
}

# First we run the program 50 times in IEEE mode
export VFC_BACKENDS="libinterflop_mca.so --mode ieee"
gcov_report
cp unst_branch.c.gcov unst_branch.gcov.IEEE

# First we run the program 50 times in RR 53 mode
export VFC_BACKENDS="libinterflop_mca.so --mode rr --precision-binary64 53"
gcov_report
cp unst_branch.c.gcov unst_branch.gcov.RR53

if diff -u unst_branch.gcov.IEEE unst_branch.gcov.RR53; then
  echo
  echo "No unstable branch detected. Test failed."
  exit 1
else
  echo
  echo "Unstable branch detected!"
  exit 0
fi
