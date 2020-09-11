#!/usr/bin/env python3

# np.random.uniform has been deprecated in favor of random.default_rng().uniform
# starting with numpy 1.19. We are keeping the old interface to account for older
# setups but this should be replaced at some point.

import numpy as np
import sys

output_filename = "input.txt"

def print_random_fp(n, r, output_filename):

    #the function will generate a minimum of 3 numbers
    n_s=int(max(n/3,1))
    reminder=max(0,n-3*n_s)
    #cannot exceed float exponent because of numpy random uniform
    #furthermore we are generating float which means when testing double that the mantissa will have zero's after bit 23, which is not great to test rounding...
    emax = 2**(r-1)-1.0
    #emax = min(2**(r-1)-1,127)
    #any number in the representable range
    rand_fp_array_1 = np.random.uniform(low=-2**(emax-1),high=2**(emax-1), size=n_s+reminder)
    rand_fp_array_2 = np.random.uniform(low=-2**(emax-1),high=2**(emax-1), size=n_s+reminder)

    #add small numbers with negative exponent
    rand_small_fp_array_1 = np.random.uniform(low=-1,high=1, size=n_s)
    rand_small_fp_array_2 = np.random.uniform(low=-1,high=1, size=n_s)

    #add denormals
    emin=1.0-emax;
    rand_sub_fp_array_1 = np.random.uniform(low=-2**(emin),high=2**(emin), size=n_s)
    rand_sub_fp_array_2 = np.random.uniform(low=-2**(emin),high=2**(emin), size=n_s)

    fo = open(output_filename, "w")

    for fp1,fp2 in zip(rand_fp_array_1,rand_fp_array_2):
        fo.write("{f1} {f2}\n".format(f1=fp1.hex(), f2=fp2.hex()))
    for fp1,fp2 in zip(rand_small_fp_array_1,rand_small_fp_array_2):
        fo.write("{f1} {f2}\n".format(f1=fp1.hex(), f2=fp2.hex()))
    for fp1,fp2 in zip(rand_sub_fp_array_1,rand_sub_fp_array_2):
        fo.write("{f1} {f2}\n".format(f1=fp1.hex(), f2=fp2.hex()))

    fo.close()

if "__main__" == __name__:

    n = int(sys.argv[1])
    r = int(sys.argv[2])
    print_random_fp(n, r, output_filename)
