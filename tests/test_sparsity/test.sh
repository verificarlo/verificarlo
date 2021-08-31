#!/bin/bash
#set -e

Check() {
  START="$(date +%s%N)"
  for i in `seq 1000` ; do
      export VFC_BACKENDS="libinterflop_mca.so --mode=rr --precision-binary64=1 --precision-binary32=1 --sparsity=${sparsity} --seed=${i}"
      ./test 0x1.fffffffffffffp2  0x1.fffffffffffffp-50 >> log 2> /dev/null
  done
  DURATION=$[ ($(date +%s%N) - ${START})/1000000 ]
  perc=`./parse.py log ${sparsity}`
  if [ $? -eq 1 ]; then
    ecode=1
  fi
  echo "  Sparsity: ${sparsity} | ${perc}% IEEE (Time Elapsed: $DURATION ms)"
}

export VFC_BACKENDS_SILENT_LOAD=True
export VFC_BACKENDS_LOGGER=False
ecode=0

for dtype in "float" "double"; do
  echo $dtype | awk '{ print toupper($0) }'
  verificarlo-c -D REAL=${dtype} -O0 test.c -o test
  for sparsity in 1 0.9 0.5 0.25 0.1 0.01; do    
    rm -f log
    Check
  done
done

exit $ecode
