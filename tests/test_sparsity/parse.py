#!/bin/env python3

from argparse import ArgumentParser
import numpy as np
from collections import Counter

if __name__ == '__main__':
    parser = ArgumentParser()
    parser.add_argument("logfile")
    parser.add_argument("sparsity", type=int)

    args = parser.parse_args()

    x = np.loadtxt(args.logfile)
    c = Counter(x)

    percent_ieee = c[8]*100.0/len(x)
    print(percent_ieee)
    
    rounded=round(percent_ieee, 0)
    assert(rounded == 100*(1 - 1.0/args.sparsity)) 

