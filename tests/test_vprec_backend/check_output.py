#!/usr/bin/python3

import os
import math
import sys

exit_at_error = False

PRECISION="VERIFICARLO_PRECISION"
mpfr_file="mpfr.txt"
vprec_file="vprec.txt"

def get_var_env_int(env):

    varenv = os.getenv(env)
    if varenv and varenv.isdigit():
        return int(varenv)
    else:
        print("Bad {env} {varenv}".format(env=env, varenv=varenv))
        exit(1)

def parse_file(filename):
    fi = open(filename, "r")
    fp_list = []
    return [float.fromhex(line) for line in fi]

def get_relative_error(mpfr, vprec):

    if math.isnan(mpfr) != math.isnan(vprec):
        print("Error NaN")
        exit(1)
    elif math.isinf(mpfr) != math.isinf(vprec):
        print("Error Inf")
        exit(1)
    elif math.isnan(mpfr) and math.isnan(vprec):
        return -1
    elif mpfr == vprec:
        return -1
    elif mpfr == 0.0:
        return abs(vprec)
    elif vprec == 0.0:
        return abs(mpfr)
    else:
        return abs((mpfr-vprec)/mpfr)

def get_significant_digits(relative_error):
    # Special case when mpfr == vprec
    # return high significance
    if relative_error == -1:
        return 100
    else:
        return abs(math.log(relative_error,2))

def are_equal(mpfr, vprec):
    if math.isnan(mpfr) and math.isnan(vprec):
        return True
    else:
        return mpfr == vprec

def compute_err(precision, mpfr_list, vprec_list):
    for mpfr,vprec in zip(mpfr_list,vprec_list):
        print("Compare MPFR,VPREC:{mpfr} {vprec}".format(mpfr=mpfr,vprec=vprec))
        relative_error = get_relative_error(mpfr, vprec)
        s = get_significant_digits(relative_error)
        if math.ceil(s) < precision:
            float_type=os.getenv('VERIFICARLO_VPREC_TYPE')
            vprec_range=os.getenv('VERIFICARLO_VPREC_RANGE')
            vprec_precision=os.getenv('VERIFICARLO_PRECISION')
            vprec_mode=os.getenv('VERIFICARLO_VPREC_MODE')
            op=os.getenv('VERIFICARLO_OP')
            sys.stderr.write("{t}: MODE={m} RANGE={r} PRECISION={p} OP={op}\n".format(
                t=float_type,
                m=vprec_mode,
                r=vprec_range,
                p=vprec_precision,
                op=op))

            sys.stderr.write("Relative error too high: mpfr={mpfr} vprec={vprec} error={err} ({el} b=2)\n".format(
                mpfr=mpfr,
                vprec=vprec,
                err=relative_error,
                el=s))

            sys.stderr.flush()
            if exit_at_error:
                exit(1)

if "__main__" == __name__:

    precision = get_var_env_int(PRECISION)
    mpfr_list = parse_file(mpfr_file)
    vprec_list = parse_file(vprec_file)

    compute_err(precision, mpfr_list, vprec_list)

    exit(0)
