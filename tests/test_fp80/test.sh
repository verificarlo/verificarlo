#/bin/bash
set -e

# Verificarlo should not fail
verificarlo -O2 -c rotg.c 2> error

# But is should report the unsupported type
if grep "Unsupported operand type: x86_fp80" error; then
  echo "verificarlo correctly reported unsupported x86_fp80 type"
  exit 0
else
  echo "verificarlo failed silently"
  exit 1
fi
