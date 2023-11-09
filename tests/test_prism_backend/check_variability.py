#!/bin/env python3

import numpy as np

np.seterr(all="ignore")
import argparse
import sys
import os
import re
import significantdigits as sd

def read_file(filename):
    basename = os.path.basename(filename)
    vecsize = re.search(r'\.x(\d+)', basename)
    if vecsize:
        vecsize = int(vecsize.group(1))
    else:
        vecsize = 1

    conv = {i : float.fromhex for i in range(vecsize)}
    return np.genfromtxt(filename, converters=conv, autostrip=True, encoding='utf-8')


def compute_sig(x, verbose):
    mean = x.mean(axis=0)
    std = x.std(axis=0, dtype=np.float64)
    sig = sd.significant_digits(x, reference=mean, basis=10)
    sig_min = np.min(sig)
    mean_min = np.fabs(mean).min()
    if verbose:
        print(f"Mean: {mean}")
        print(f"Min mean: ", mean_min.hex())
        print(f"Std: {std}")
       print(f"Signficant digits: {sig}")
       print(f"Min significant digits: {sig_min}")
    return sig_min, mean_min, std
    return mean_min, std


def deduce_type(filename, fp_type):
    if "float" in filename or fp_type == "float":
        return "float"
    if "double" in filename or fp_type == "double":
        return "double"
    raise Exception("Invalid floating-point type" + fp_type)


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("input", type=str)
    parser.add_argument("--type", help="floating-point type of inputs")
    parser.add_argument("--verbose", action="store_true", help="verbose mode")
    return parser.parse_args()


def main():
    args = parse_args()

    try:
        fp_type = deduce_type(args.input, args.type)
        x = read_file(args.input)
        sig_min, mean_min, std = compute_sig(x, args.verbose)
    except Exception as e:
        print(f"Error with {args.input}")
        print(e)
        sys.exit(1)

    tol = 0
    if fp_type == "float":
        tol = 6
    elif fp_type == "double":
        tol = 14

    if args.verbose:
        print(f"Tolerance: {tol}")

    if sig_min < tol:
        print(
            f"Error with {args.input}: minimal number of significant digits {sig_min:.2f} below threshold ({tol:.2f})"
        )
        sys.exit(1)

    if std.max() == 0:
        print(f"Error with {args.input}: standard deviation is zero")
        sys.exit(1)

    if mean_min == 0:
        print(f"Error with {args.input}: one element is zero")
        sys.exit(1)
    if np.isnan(mean_min):
        print(f"Error with {args.input}: one element is nan")
        sys.exit(1)
    if np.isinf(mean_min):
        print(f"Error with {args.input}: one element is inf")
        sys.exit(1)


if "__main__" == __name__:

    main()
