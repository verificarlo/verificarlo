#/bin/bash
set -e

# Verificarlo should not fail
verificarlo-c -O2 -c rotg.c 2> error

if [[ $(arch) == "x86_64" ]]; then
    # But is should report the unsupported FP80 type on x86 
    if grep "Unsupported operand type: x86_fp80" error; then
      echo "verificarlo-c correctly reported unsupported x86_fp80 type"
      exit 0
    else
      echo "verificarlo-c failed silently"
      exit 1
    fi
fi
