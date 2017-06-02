#!/usr/bin/python

import math
import struct
import numpy as np
import sys
import os
    
def parse_file(filename):
    return np.loadtxt(filename)
    
def compute_ns(value):

    mean = np.mean(value)
    std = np.std(value)
        
    try:
        if mean != 0.0 and std != 0.0:
            return -math.log10(abs(std/mean))
        else:
            return 16

    except:
        print value
        exit(1)
        
if "__main__" == __name__:
    filename = sys.argv[1]
    
    values = parse_file(filename)
    
    list_ns = [compute_ns(value) for value in values]
    
    sys.stdout.write("{:f} ".format(np.mean(list_ns)))
