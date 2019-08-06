#!/usr/bin/env python

THRESHOLD = 2

import sys

max = -1

for quad, mpfr in zip(file("out_quad"), file("out_mpfr")):
    q = float(quad)
    m = float(mpfr)
    delta = abs(q-m)
    if delta > THRESHOLD:
	max = delta
        print "ERROR:", q, m

if max > THRESHOLD:
        print "MAX > THRESHOLD"
	sys.exit(1)

sys.exit(0)
