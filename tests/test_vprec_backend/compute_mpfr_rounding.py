#!/usr/bin/env python3

from bigfloat import (BigFloat, Context,
                      ROUND_TIES_TO_EVEN, setcontext,
                      single_precision, double_precision, quadruple_precision,
                      add, sub, mul, div, pos)
import sys
import os
import math
import functools
from enum import Enum

verbose_mode = False
VERBOSE_MODE = "VERBOSE_MODE"


class OperatorName(Enum):
    Add = "+"
    Sub = "-"
    Mul = "x"
    Div = "/"


class Operator(Enum):
    Add = add
    Sub = sub
    Mul = mul
    Div = div


class VprecMode(Enum):
    IEEE = "IEEE"
    PB = "IB"
    OB = "OB"
    FULL = "FULL"


VPREC_RANGE = "VERIFICARLO_VPREC_RANGE"
PRECISION = "VERIFICARLO_PRECISION"
VPREC_MODE = "VERIFICARLO_VPREC_MODE"


def error(msg):
    print(msg, file=sys.stderr)
    sys.exit(1)


def parse_file(filename):
    with open(filename, "r", encoding='utf-8') as fi:
        return [BigFloat(fp.strip()) for fp in fi]


def get_operator(op):
    if op == OperatorName.Add.value:
        return Operator.Add
    if op == OperatorName.Sub.value:
        return Operator.Sub
    if op == OperatorName.Mul.value:
        return Operator.Mul
    if op == OperatorName.Div.value:
        return Operator.Div
    error(f"Bad op {op}")
    return None


def get_var_env(env, apply=lambda x: x):
    varenv = os.getenv(env)
    try:
        return apply(varenv)
    except Exception as e:
        error(f"Bad {env} {varenv}: {e}")
        return None


def get_custom_context(vrange, prec):
    emax = 2**(vrange - 1)
    emin = -2**(vrange - 1) - prec + 4

    context = Context(precision=prec,
                      emin=emin,
                      emax=emax,
                      subnormalize=True,
                      rounding=ROUND_TIES_TO_EVEN)

    return context


def get_context_type(float_type):
    if float_type == "float":
        return single_precision
    if float_type == "double":
        return double_precision
    error(f"Unknown type {float_type}")
    return None

# 0x0.xp+e


def hex_normalize(flt):
    if math.isinf(flt) or math.isnan(flt) or flt == 0.0:
        return str(flt)

    hex_str = flt.hex()
    s = functools.reduce(lambda s, f: s.replace(
        f, '.'), ['0x', '.', 'p'], hex_str)
    s = s.split('.')

    try:
        sign = '+'
        (_, frac, exp) = filter(lambda x: x != '', s)
    except ValueError:
        (sign, _, frac, exp) = filter(lambda x: x != '', s)
    except Exception as e:
        print(e)
        print(flt, s)
        sys.exit(1)

    mantissa = str(hex(int(frac, 16) << 1))
    mantissa = mantissa.replace('1', '1.', 1)
    exp = int(exp) - 1
    signe_exp = '+' if exp >= 0 else ''
    return f"{sign}{mantissa}p{signe_exp}{exp}"


def print_mpfr_hex(name, mpfr_x, msg='', verbose=False):
    if verbose:
        print(f"[MPFR] {msg} {name}={hex_normalize(mpfr_x)}", file=sys.stderr)


def get_verbose_mode():
    return os.getenv(VERBOSE_MODE) is not None


def parse_args():

    if len(sys.argv) != 5:
        print("4 arguments expected: a b op context")
        sys.exit(1)

    verbose = get_verbose_mode()

    a = sys.argv[1]
    b = sys.argv[2]
    op = get_operator(sys.argv[3])
    context = get_context_type(sys.argv[4])

    return dict(x=a, y=b, operator=op, context=context, verbose=verbose)


def main():

    args = parse_args()
    a = args['x']
    b = args['y']
    op = args['operator']
    context = args['context']
    verbose = args['verbose']

    vrange = get_var_env(VPREC_RANGE, int)
    prec = get_var_env(PRECISION, int) + 1
    mode = get_var_env(VPREC_MODE)

    setcontext(context)

    if verbose:
        print(f'{VPREC_RANGE}={vrange}', file=sys.stderr)
        print(f'{PRECISION}={prec}', file=sys.stderr)
        print(f'{VPREC_MODE}={mode}', file=sys.stderr)
        print(f'a={a}', file=sys.stderr)
        print(f'b={b}', file=sys.stderr)
        print(f'op={op}', file=sys.stderr)
        print(f'context={context}', file=sys.stderr)

    mpfr_a = BigFloat.fromhex(a)
    mpfr_b = BigFloat.fromhex(b)
    print_mpfr_hex('a', mpfr_a, '(before rounding)', verbose=verbose)
    print_mpfr_hex('b', mpfr_b, '(before rounding)', verbose=verbose)

    if mode in (VprecMode.PB.value, VprecMode.FULL.value):
        context = get_custom_context(vrange, prec)
        mpfr_a = pos(mpfr_a, context=context)
        mpfr_b = pos(mpfr_b, context=context)

    res = op(mpfr_a, mpfr_b, context=quadruple_precision)

    if mode in (VprecMode.OB.value, VprecMode.FULL.name):
        context = get_custom_context(vrange, prec)

    mpfr_res = pos(res, context=context)

    print_mpfr_hex('a', mpfr_a, '(after rounding)', verbose=verbose)
    print_mpfr_hex('b', mpfr_b, '(after rounding)', verbose=verbose)
    print_mpfr_hex('res', mpfr_res, verbose=verbose)

    print(hex_normalize(mpfr_res))


if __name__ == "__main__":
    main()
