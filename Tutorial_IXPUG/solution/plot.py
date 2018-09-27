#!/usr/bin/python

############################################################################
# Plots mean (mu), standard deviation (sigma), and significant digits (s)  #
# from a set of stochastic outputs from tchebychev.                        #
############################################################################

import sys
import re
import numpy as np
import matplotlib.pyplot as plt
import math

# Read command line arguments
if len(sys.argv) != 3 or not sys.argv[1].endswith('.tab'):
    print("usage: ./plot_all.py DATA.tab precision")
    sys.exit(1)

fname = sys.argv[1]
prec_b = sys.argv[2]
version = fname[:-4]

# Convert binary to decimal precision
prec_dec=float(prec_b)*math.log(2, 10)

# Parse table file
# three columns:
#   - i: sample number
#   - x: input value
#   - T: polynomial evaluation on x, T(x)
D = np.loadtxt(fname, skiprows=1,
               dtype=dict(names=('i', 'x', 'T'),
                          formats=('i4', 'f8', 'f8')))

# Compute all statistics (mu, sigma, s)
x_values = np.unique(D['x'])
mu_values = []
sigma_values = []
s_values = []

for x in x_values:
    # select all T samples for given x
    T_samples = D[D['x'] == x]['T']

    # Compute mu and sigma statistics
    mu = np.mean(T_samples)
    sigma = np.std(T_samples)

    # Compute significant digits
    if sigma == 0:
        s = prec_dec
    elif mu == 0:
        s = 0
    else:
        # Stott Parker's formula
        s = min(-math.log10(sigma/abs(mu)), prec_dec)

    mu_values.append(mu)
    sigma_values.append(sigma)
    s_values.append(s)


x_err = []
y_err = []
with open(fname[0:-3]+"err") as f:
    for line in f:
        cols = line.split()
        x_err.append(float(cols[0]))

        err = float(cols[3])
        s = min(-math.log10(err), prec_dec)
        y_err.append(s)


# Plot all statistics
plt.style.use('bmh')

# Set title
title=version + " verificarlo precision = " + prec_b + "bits"
plt.figure(title, figsize=(10,8))
plt.suptitle(title)

# Plot significant digits
plt.subplot(311)
plt.ylabel("$s$")
plt.plot(x_values, s_values, '.')
plt.plot(x_err,    y_err, '+', color='g')

# Plot standard deviation
plt.subplot(312)
plt.ylabel("$\hat \sigma$")
plt.plot(x_values, sigma_values, '.')

# Plot samples and mean
plt.subplot(313)
plt.xlabel("$x$")
plt.ylabel("$T(x)$ and $\hat \mu$")
plt.plot(D['x'], D['T'], 'k.', alpha=0.5)
plt.plot(x_values, mu_values, '--', color='r')

# Set layout
plt.tight_layout()
plt.subplots_adjust(top=0.9)

# Save plot as pdf
plotname=version+"-"+prec_b+".pdf"
plt.savefig(plotname, format='pdf')

#plt.show()
