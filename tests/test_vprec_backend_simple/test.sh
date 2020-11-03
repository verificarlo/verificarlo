#!/bin/sh
set -xe


## Denormal tests for range 2, precision 3
cat > input.txt << EOF
0x1.a53a8b6373154p-1 0x1.cfa1850291880p-4
-0x1.6312bf6a7f2f8p-1 -0x1.5b403af9711e8p-3
0x1.e4f85f0e582c0p-1 0x1.4b57c729ba5e0p-3
0x1.9d6aa71f9cb18p-1 0x1.9fdf01e9551c8p-1
0x1.8p-1 0x1.8p-1
0x1.f41e5c48bb0b8p-2 -0x1.2df365dba47ecp-1
EOF

./compare.sh OB 2 3 float x input.txt
./compare.sh FULL 2 3 float x input.txt

## Denormal tests for range 2, precision 23
cat > input.txt << EOF
-0x1.eee201b1c85d4p-1 0x1.22034fafd4a10p-4
EOF
./compare.sh FULL 2 23 float x input.txt

