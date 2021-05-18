#!/bin/sh
set -xe

# Input low precicsion
cat > input_low.txt << EOF
0x1.a53a8b6373154p-1 0x1.cfa1850291880p-4
-0x1.6312bf6a7f2f8p-1 -0x1.5b403af9711e8p-3
0x1.e4f85f0e582c0p-1 0x1.4b57c729ba5e0p-3
0x1.9d6aa71f9cb18p-1 0x1.9fdf01e9551c8p-1
0x1.8p-1 0x1.8p-1
0x1.f41e5c48bb0b8p-2 -0x1.2df365dba47ecp-1
EOF

# Input high precision
cat > input_high.txt << EOF
-0x1.eee201b1c85d4p-1 0x1.22034fafd4a10p-4
0x1.22034fafd4a10p-4 -0x1.eee201b1c85d4p-1
EOF

# Scalar
echo "[SCALAR]"

## Denormal tests for range 2, precision 3
./compare.sh OB 2 3 float x input_low.txt 0
./compare.sh FULL 2 3 float x input_low.txt 0

## Denormal tests for range 2, precision 23
./compare.sh FULL 2 23 float x input_high.txt 0

# Vector float
echo "[VECTOR FLOAT]"

## Denormal tests for range 2, precision 3
echo "[DENORMAL]"

./compare.sh OB 2 3 float x input_low.txt 1
./compare.sh FULL 2 3 float x input_low.txt 1

## Denormal tests for range 2, precision 23
echo "[DENORMAL]"

./compare.sh FULL 2 23 float x input_high.txt 1

## Normal test for range 6, precision 23
echo "[NORMAL]"
cat > input.txt << EOF
0x1.a53a8b6373154p1 0x1.cfa1850291880p-1
-0x1.6312bf6a7f2f8p-0 -0x1.5b403af9711e8p-1
EOF

./compare.sh FULL 6 23 float x input.txt 1

# Vector double
echo "[VECTOR DOUBLE]"

## Denormal tests for range 2, precision 3
echo "[DENORMAL]"

./compare.sh OB 2 3 double x input_low.txt 1
./compare.sh FULL 2 3 double x input_low.txt 1

## Denormal tests for range 2, precision 52
echo "[DENORMAL]"

./compare.sh FULL 2 52 double x input_high.txt 1

## Normal tests for range 6, precision 52
echo "[NORMAL]"
cat > input.txt << EOF
0x1.a53a8b6373154p1 0x1.cfa1850291880p-1
-0x1.6312bf6a7f2f8p-0 -0x1.5b403af9711e8p-1
EOF

./compare.sh FULL 6 52 double x input.txt 1

