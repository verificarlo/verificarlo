#!/usr/bin/python

import numpy as np
import sys

def parse_file(filename):
    return np.loadtxt(filename)
    
def compute_std(value):
    std = np.std(value)
    if std > 0.1:
        print "Error std : " + str(value) + " " + str(std)
        exit(1)
        
if "__main__" == __name__:
    filename = sys.argv[1]

    values = parse_file(filename)
    
    for value in values:
        compute_std(value)
