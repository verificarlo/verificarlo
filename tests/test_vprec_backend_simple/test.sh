#!/bin/sh

# Denormal tests for range 2, precision 3
cat > input.txt << EOF
0x1.a53a8b6373154p-1 0x1.cfa1850291880p-4
-0x1.6312bf6a7f2f8p-1 -0x1.5b403af9711e8p-3
0x1.e4f85f0e582c0p-1 0x1.4b57c729ba5e0p-3
EOF

./compare.sh OB 2 3 float x input.txt
