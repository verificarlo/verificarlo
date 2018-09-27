#!/usr/bin/python

from fractions import Fraction
from mpmath import iv, mpf
import sys, math

coefs = [
    1,
    -200,
    6600,
    -84480,
    549120,
    -2050048,
    4659200,
    -6553600,
    5570560,
    -2621440,
    524288
]

def expanded (typ, x):
    x2 = typ(x)*typ(x)

    r = typ(coefs[0])
    xp = x2
    for i in xrange(1, len(coefs)):
        r += typ(coefs[i]) * xp
        xp *= x2

    return r

def to_float(x):
    if isinstance(x, iv.mpf):
        a = float(mpf(x.a))
        b = float(mpf(x.b))
        if a == b:
            return a
        else:
            raise ValueError("Interval %s is too large to be converted to float\n delta/mid ~ %s" % (x, 2*(b-a)/(b+a)))

    return float(x)

iv.dps = 33

for line in sys.stdin:
    (x, res) = [float(s) for s in line.split()]
    ref = expanded(Fraction, x)
    ref2 = expanded(iv.mpf, x)

    if to_float(ref) != to_float(ref2):
        raise AssertionError("Fraction != iv.mpf")

    err = to_float (abs((res-ref)/ref))
    ref = to_float (ref)
    try:
        s = -math.log10(err)
        print "%.18e %.18e %.18e %.2e %.2f" % (x, res, ref, err, s)
    except:
        pass
