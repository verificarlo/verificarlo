#!/usr/bin/env python3

import numpy as np
import sys

_output_filename = "input.txt"

uniform = np.random.default_rng().uniform


def get_normal_number(emax, reminder, n_s):
    # any number in the representable range
    low = -2**(emax - 1)
    high = 2**(emax - 1)
    size = n_s + reminder
    a1 = uniform(low=low, high=high, size=size)
    a2 = uniform(low=low, high=high, size=size)
    return (a1, a2)


def get_small_number(n_s):
    # add small numbers with negative exponent
    a1 = uniform(low=-1, high=1, size=n_s)
    a2 = uniform(low=-1, high=1, size=n_s)
    return (a1, a2)


def get_subnormal_number(emax, n_s):
    # add denormals
    emin = 1.0 - emax
    low = -2**(emin)
    high = 2**(emin)
    size = n_s
    a1 = uniform(low=low, high=high, size=size)
    a2 = uniform(low=low, high=high, size=size)
    return (a1, a2)


def print_random_fp(n, r, output_filename):

    # the function will generate a minimum of 3 numbers
    n_s = int(max(n / 3, 1))
    reminder = max(0, n - 3 * n_s)
    # cannot exceed float exponent because of numpy random uniform
    # furthermore we are generating float which means when testing
    # double that the mantissa will have zero's after bit 23,
    # which is not great to test rounding...
    emax = 2**(r - 1) - 1.0
    # emax = min(2**(r-1)-1,127)

    # any number in the representable range
    rand_fp_array_1, rand_fp_array_2 = get_normal_number(emax, reminder, n_s)

    # add small numbers with negative exponent
    rand_small_fp_array_1, rand_small_fp_array_2 = get_small_number(n_s)

    # add denormals
    rand_sub_fp_array_1, rand_sub_fp_array_2 = get_subnormal_number(emax, n_s)

    with open(output_filename, "w", encoding='utf-8') as fo:
        for fp1, fp2 in zip(rand_fp_array_1, rand_fp_array_2):
            fo.write(f"{fp1.hex()} {fp2.hex()}\n")
        for fp1, fp2 in zip(rand_small_fp_array_1, rand_small_fp_array_2):
            fo.write(f"{fp1.hex()} {fp2.hex()}\n")
        for fp1, fp2 in zip(rand_sub_fp_array_1, rand_sub_fp_array_2):
            fo.write(f"{fp1.hex()} {fp2.hex()}\n")


def main():
    n = int(sys.argv[1])
    r = int(sys.argv[2])
    print_random_fp(n, r, _output_filename)


if "__main__" == __name__:
    main()
