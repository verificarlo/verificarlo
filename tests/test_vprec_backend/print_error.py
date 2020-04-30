#!/usr/bin/python

import sys

if '__main__' == __name__:

    inputs_filename = sys.argv[1]
    mpfr_filename   = sys.argv[2]
    vprec_filename  = sys.argv[3]

    inputs_values = [fi.strip() for fi in open(inputs_filename)]
    mpfr_values   = [fi.strip() for fi in open(mpfr_filename)]
    vprec_values  = [fi.strip() for fi in open(vprec_filename)]
    
    nb_values = len(inputs_values)
    
    for i in range(nb_values):
        x,y = map(float.fromhex, inputs_values[i].split())
        print inputs_values[i],mpfr_values[i],vprec_values[i]," # Double result ",float(x*y).hex()

        
