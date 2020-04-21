#!/usr/bin/python3

import numpy as np
import sys

output_filename = "input.txt"

def print_random_fp(n, r, output_filename):

    emax = 2**(r-1)-1
    rand_fp_array_1 = np.random.normal(loc=0, scale=2**emax, size=n)
    rand_fp_array_2 = np.random.normal(loc=0, scale=2**emax, size=n)

    fo = open(output_filename, "w")

    for fp1,fp2 in zip(rand_fp_array_1,rand_fp_array_2):
        fo.write("{f1} {f2}\n".format(f1=fp1.hex(), f2=fp2.hex()))

    fo.close()

if "__main__" == __name__:

    n = int(sys.argv[1])
    r = int(sys.argv[1])

    print_random_fp(n, r, output_filename)
