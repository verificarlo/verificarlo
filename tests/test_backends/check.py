#!/usr/bin/env python

THRESHOLD = 1.9

import sys

for quad, mpfr in zip(file("out_quad"), file("out_mpfr")):
    q = float(quad)
    m = float(mpfr)
    if q-m > THRESHOLD:
        print "ERROR:", q, m
        sys.exit(1)

sys.exit(0)
