#!/usr/bin/env python3

from scipy.stats import binomtest
from collections import Counter


def freq(l):
    """ return dictionnary of frequencies """
    return Counter(l)


def read(name):
    """ read filename and returns the distribution of outputs as a dictionnary """
    with open(name, "r", encoding='utf-8') as f:
        data = [x.strip() for x in f.readlines()]
        freqs = freq(data)
    return freqs


confidence_level = 0.99


def binomial_test_ci(success, trials, proba_success, confidence=confidence_level):
    '''
    Compute confidence interval for a Bernouilli trial
    Check if obtained result is in the interval
    '''
    low, high = binomtest(k=success, n=trials,
                          p=proba_success).proportion_ci(confidence)
    return low < success / trials < high


if __name__ == "__main__":
    import sys
    assert(len(sys.argv) == 2)
    its = int(sys.argv[1])
    f = read("binary32-23")
    assert(len(f) == 1)
    assert(f['0x1.4000020000000p+0'] == its)
    # For inexact24(1.25 + 2**-23) since the computed value is exact in 24 bits,
    # SR (MCA RR 24) should preserve the value

    f = read("binary32-24")
    assert(len(f) == 2)
    s = f['0x1.4000000000000p+0']
    assert(binomial_test_ci(s, its, 0.5))
    # For inexact24(1.25 + 2**-24) we are in the middle of the interval
    # SR (MCA RR 24) should give us with 50% chance one of the two
    # upper and lower bounds 0x1.4000000000000p+0 and 0x1.4000020000000p+0

    f = read("binary32-25")
    assert(len(f) == 2)
    s = f['0x1.4000000000000p+0']
    assert(binomial_test_ci(s, its, 0.75))
    # For inexact24(1.25 + 2**-25) we are in the quarter of the interval
    # SR (MCA RR 24) should give us
    # upper and lower bounds 0x1.4000000000000p+0 and 0x1.4000020000000p+0
    # with probabilities 75% and 25% respectively

    f = read("binary32-26")
    assert(len(f) == 2)
    s = f['0x1.4000000000000p+0']
    assert(binomial_test_ci(s, its, 0.875))
    # For inexact24(1.25 + 2**-26) we are in the eight of the interval
    # SR (MCA RR 24) should give us
    # upper and lower bounds 0x1.4000000000000p+0 and 0x1.4000020000000p+0
    # with probabilities 87.5% and 12.5% respectively

    # Assert similar properties for double precision
    f = read("binary64-52")
    assert(len(f) == 1)
    s = f['0x1.4000000000000p+0']
    assert(f['0x1.4000000000001p+0'] == its)

    f = read("binary64-53")
    assert(len(f) == 2)
    s = f['0x1.4000000000000p+0']
    assert(binomial_test_ci(s, its, 0.5))

    f = read("binary64-54")
    assert(len(f) == 2)
    s = f['0x1.4000000000000p+0']
    assert(binomial_test_ci(s, its, 0.75))

    f = read("binary64-55")
    assert(len(f) == 2)
    s = f['0x1.4000000000000p+0']
    assert(binomial_test_ci(s, its, 0.875))
