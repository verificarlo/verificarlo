#!/bin/env python3

import numpy as np
from collections import Counter

if __name__ == '__main__':
    x = np.loadtxt('log')
    c = Counter(x)
    print(c[8])
