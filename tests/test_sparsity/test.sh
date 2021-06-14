#!/bin/bash
#set -e

Check() {
  START="$(date +%s%N)"
  for i in `seq 1000` ; do
      ./test 0x1.fffffffffffffp2  0x1.fffffffffffffp-50 ${dtype} >> log 2> /dev/null
  done
  DURATION=$[ ($(date +%s%N) - ${START})/1000000 ]
  perc=`./parse.py log ${sparsity}`
  if [ $? -eq 1 ]; then
    ecode=1
  fi
  echo "  Sparsity: ${sparsity} | ${perc}% IEEE (Time Elapsed: $DURATION ms)"
}

verificarlo-c -O0 test.c -o test
export VFC_BACKENDS_SILENT_LOAD=True
export VFC_BACKENDS_LOGGER=False
ecode=0

for dtype in "float" "double"; do
  echo $dtype | awk '{ print toupper($0) }'
  for sparsity in 1 2 4 10 100; do    
    rm -f log
    export VFC_BACKENDS="libinterflop_mca.so --mode=rr --precision-binary64=1 --precision-binary32=1 --sparsity=${sparsity}"
    Check
  done
done

exit $ecode
