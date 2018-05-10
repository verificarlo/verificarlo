#!/usr/bin/env python

import math

SDN_MAX = {4 : 24, 8 : 52}

def log_base_convert(x, from_base, to_base):
    return x * math.log(from_base, to_base)

def std(mean, x):
    ln = len(x)
    if ln < 2:
        return 0.0
    n = 0
    m = mean
    M2 = 0.0
    for xi in x:
        n += 1
        delta = xi - m
        m += delta / n
        M2 += delta*(xi - m)
    return math.sqrt(M2/(ln-1))

def median(x):
    ln = len(x)
    if ln == 1:
        return x[0]
    else:
        s = sorted(x)    
        s_odd = s[(ln-1)/2]
        if ln % 2 == 0:
            return (s_odd+s[ln/2])/2
        else:
            return s_odd

def mean(list_FP):
    return math.fsum(list_FP) / len(list_FP)

# Compute the number of significant digits following definition given by Parker
# s = - log_base | sigma / mu |, where base is the calculating basis
def sdn(mean, std, size_bytes, base=10):
    if std != 0.0:
        if mean != 0.0:
            return -math.log(abs(std/mean),base)
        else:
            return 0
    else: # std == 0.0
        return log_base_convert(SDN_MAX[size_bytes], 2, base)

# Translate from C99 hexa representation to FP
# 0x+/-1.mp+e, m mantissa and e exponent
def hexa_to_fp(hexafp):
    return float.fromhex(hexafp)
