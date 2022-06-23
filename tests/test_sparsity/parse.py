#!/usr/bin/env python3

from argparse import ArgumentParser
import numpy as np
from collections import Counter

if __name__ == '__main__':
    parser = ArgumentParser()
    parser.add_argument("logfile")
    parser.add_argument("sparsity", type=float)

    args = parser.parse_args()

    # The previous version:
    # x = np.loadtxt(args.logfile)
    # seems broken in recent version, therefore we do the
    # float parsing manually.
    x = [float.fromhex(l.strip()) for l in open(args.logfile)]

    c = Counter(x)

    percent_ieee = c[8]*100.0/len(x)
    print(percent_ieee)

    assert(np.isclose(percent_ieee, 100*(1-args.sparsity), atol=2e-1))

