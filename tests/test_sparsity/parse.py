#!/usr/bin/env python3

from argparse import ArgumentParser
import numpy as np
from collections import Counter

if __name__ == '__main__':
    parser = ArgumentParser()
    parser.add_argument("logfile")
    parser.add_argument("sparsity", type=float)

    args = parser.parse_args()

    x = np.loadtxt(args.logfile)
    c = Counter(x)

    percent_ieee = c[8]*100.0/len(x)
    print(percent_ieee)

    assert(np.isclose(percent_ieee, 100*(1-args.sparsity), atol=2e-1))

