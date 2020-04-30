#!/usr/bin/python3

from bigfloat import *
import sys
import os
import math
import functools

verbose_mode=False
VERBOSE_MODE="VERBOSE_MODE"

op_list = ["+","-","x","/"]
op2bfOp = { "+" : add,
            "-" : sub,
            "x" : mul,
            "/" : div  }

VPREC_RANGE="VERIFICARLO_VPREC_RANGE"
PRECISION  ="VERIFICARLO_PRECISION"
VPREC_MODE="VERIFICARLO_VPREC_MODE"

mode_ieee = "IEEE"
mode_pb   = "IB"
mode_ob   = "OB"
mode_full = "FULL"

def parse_file(filename):
    fi = open(filename, "r")
    return [BigFloat(fp.strip()) for fp in fi]

def check_op(op):
    if op in op_list:
        return op2bfOp[op]
    else:
        print("Bad op {op}".format(op=op))
        exit(1)

def get_var_env_int(env):

    varenv = os.getenv(env)
    if varenv and varenv.isdigit():
        return int(varenv)
    else:
        print("Bad {env} {varenv}".format(env=env, varenv=varenv))
        exit(1)

def get_var_env(env):

    varenv = os.getenv(env)
    if varenv:
        return varenv
    else:
        print("Bad {env} {varenv}".format(env=env, varenv=varenv))
        exit(1)

def set_custom_context(vrange, prec):
    emax = 2**(vrange-1)
    emin = -2**(vrange-1) - prec + 4

    context = Context(precision=prec,
                      emin=emin,
                      emax=emax,
                      subnormalize=True,
                      rounding=ROUND_TIES_TO_EVEN)

    setcontext(context)
    return context

def get_context_type(float_type):
    if float_type == "float":
        return single_precision
    elif float_type == "double":
        return double_precision
    else:
        print("Unknown type {t}".format(t=float_type))
        exit(1)

# 0x0.xp+e
def hex_normalize(flt):
    if math.isinf(flt) or math.isnan(flt) or flt==0.0:
        return str(flt)

    hex_str = flt.hex()
    s = functools.reduce(lambda s,f : s.replace(f,'.'), ['0x', '.', 'p'], hex_str)
    s = s.split('.')

    try:
        sign = '+'
        [first, frac, exp] = filter(lambda x : x != '', s)
    except ValueError:
        [sign, first, frac, exp] = filter(lambda x : x != '', s)
    except Exception as e:
        print(e)
        print(flt,s)
        exit(1)

    mantissa = str(hex(int(frac,16) << 1))
    mantissa = mantissa.replace('1','1.',1)
    exp = int(exp)-1
    signe_exp = '+' if exp >= 0 else ''
    hex_normalize = "{sign}{mant}p{signe_exp}{exp}".format(sign=sign,
                                                           mant=mantissa,
                                                           signe_exp=signe_exp,
                                                           exp=exp)
    return hex_normalize

def print_mpfr_hex(name, mpfr_x, msg=''):
    if verbose_mode:
        res = hex_normalize(mpfr_x)
        fmt = "[MPFR] {msg} {name}={r}\n".format(name=name, r=res, msg=msg)
        sys.stderr.write(fmt)

def check_verbose_mode():
    global verbose_mode
    verbose_env = os.getenv(VERBOSE_MODE)
    if verbose_env:
        verbose_mode=True

if __name__ == "__main__":

    if len(sys.argv) != 5:
        print("4 arguments expected: a b op context")
        exit(1)

    check_verbose_mode()

    a = sys.argv[1]
    b = sys.argv[2]
    op = check_op(sys.argv[3])
    context_type = get_context_type(sys.argv[4])
    setcontext(context_type)

    vrange = get_var_env_int(VPREC_RANGE)
    prec = get_var_env_int(PRECISION)+1
    mode = get_var_env(VPREC_MODE)

    # DEBUG
    mpfr_a = BigFloat.fromhex(a)
    mpfr_b = BigFloat.fromhex(b)
    print_mpfr_hex('a', mpfr_a, '(before rounding)')
    print_mpfr_hex('b', mpfr_b, '(before rounding)')

    if mode == mode_pb or mode == mode_full:
        set_custom_context(vrange, prec)
    else:
        setcontext(context_type)

    mpfr_a = BigFloat.fromhex(a)
    mpfr_b = BigFloat.fromhex(b)

    if mode == mode_ob or mode == mode_full:
        set_custom_context(vrange, prec)
    else:
        setcontext(context_type)

    res = op(mpfr_a, mpfr_b)

    print_mpfr_hex('a', mpfr_a)
    print_mpfr_hex('b', mpfr_b)
    print_mpfr_hex('res', res)

    print(hex_normalize(res))
