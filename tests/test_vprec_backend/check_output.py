#!/usr/bin/python3

import os
import math
import sys

exit_at_error = True

PRECISION="VERIFICARLO_PRECISION"
mpfr_file="mpfr.txt"
vprec_file="vprec.txt"
input_file="input.txt"

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

def parse_file2(filename):
    fi = open(filename, "r")
    input_list= []
    input_list =[list(line.split(' ')) for line in fi]
    input_list
    return input_list

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
        err=abs((mpfr-vprec)/mpfr)

    if math.isnan(err):
        print("Computed relative error is NaN")
        exit(1)
    elif math.isinf(err):
        print("Computed relative error is Inf")
        exit(1)
    else:
        return err

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

def compute_err(precision, mpfr_list, vprec_list, input_list):
    for mpfr,vprec,input_ab in zip(mpfr_list,vprec_list,input_list):
        print("Compare MPFR,VPREC:{mpfr} {vprec}".format(mpfr=mpfr,vprec=vprec))
        relative_error = get_relative_error(mpfr, vprec)
        s = get_significant_digits(relative_error)

        vprec_range=int(os.getenv('VERIFICARLO_VPREC_RANGE'))

        emin = - (1 << (vprec_range - 1))
        # Check for denormal case
        # Since we check after computation, we have to account for operands
        # being denormal and result being rounded to 2**(emin).
        # WARNING: not ties to even, if the two operands have a ties-to-even problem
        # and the result also, this check may not be enough.
        # (In OB mode this should never happen)
        if mpfr != 0 and abs(mpfr) <= 2**(emin):
            # In denormal case we acount for the precision lost
            required_prec = precision - (emin - math.ceil(math.log(abs(mpfr),2)-1))
        else:
            required_prec = precision

        # we add one bit to account for faithful rounding
        if math.ceil(s) < required_prec - 1:
            float_type=os.getenv('VERIFICARLO_VPREC_TYPE')
            vprec_precision=os.getenv('VERIFICARLO_PRECISION')
            vprec_mode=os.getenv('VERIFICARLO_VPREC_MODE')
            op=os.getenv('VERIFICARLO_OP')
            sys.stderr.write("{t}: MODE={m} RANGE={r} PRECISION={p} OP={op}\n".format(
                t=float_type,
                m=vprec_mode,
                r=vprec_range,
                p=vprec_precision,
                op=op))

            sys.stderr.write("Relative error too high: a={input_a} b={input_b} mpfr={mpfr} vprec={vprec} error={err} ({el} b=2)\n".format(
                input_a=input_ab[0],
                input_b=input_ab[1],
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
    input_list = parse_file2(input_file)
    compute_err(precision, mpfr_list, vprec_list, input_list)

    exit(0)
