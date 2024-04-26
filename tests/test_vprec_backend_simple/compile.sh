source ../paths.sh

GCC=${GCC_PATH}

${GCC} compute_mpfr_rounding.c -lmpfr -Wall -Wextra -o compute_mpfr_rounding -O3 -march=native
