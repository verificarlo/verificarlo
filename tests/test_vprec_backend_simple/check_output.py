#!/usr/bin/python3

import math
import os
import sys
from sys import exit

PRECISION = "VERIFICARLO_PRECISION"
mpfr_file = "mpfr.txt"
vprec_file = "vprec.txt"
input_file = "input.txt"


def exit_at_error():
    varenv = os.getenv("VERIFICARLO_EXIT_AT_ERROR")
    if varenv is not None:
        if varenv.isdigit() and int(varenv) == 0:
            return False
        if varenv.lower() == "false" or bool(varenv) is False:
            return False
    return True


def get_var_env_int(env):
    varenv = os.getenv(env)
    if varenv and varenv.isdigit():
        return int(varenv)
    else:
        print("Bad {env} {varenv}".format(env=env, varenv=varenv))
        exit(1)


def parse_file(filename):
    with open(filename, "r") as fi:
        return [float.fromhex(line) for line in fi]


def parse_file2(filename):
    with open(filename, "r") as fi:
        return [list(line.strip().split(" ")) for line in fi]


def get_relative_error(mpfr, vprec):
    if math.isnan(mpfr) != math.isnan(vprec):
        print("Error NaN")
        exit(1)
    elif math.isinf(mpfr) != math.isinf(vprec):
        print("Error Inf")
        exit(1)
    elif math.isnan(mpfr) and math.isnan(vprec):
        return 0.0
    elif mpfr == vprec:
        return 0.0
    elif mpfr == 0.0:
        return abs(vprec)
    elif vprec == 0.0:
        return abs(mpfr)
    else:
        err = abs((mpfr - vprec) / mpfr)

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
    if relative_error == 0.0:
        return math.inf
    else:
        return abs(math.log(relative_error, 2))


def are_equal(mpfr, vprec):
    if math.isnan(mpfr) and math.isnan(vprec):
        return True
    else:
        return mpfr == vprec


def compute_err(precision, mpfr_list, vprec_list, input_list):
    for mpfr, vprec, input_ab in zip(mpfr_list, vprec_list, input_list):
        print(f"Compare MPFR={mpfr} vs VPREC={vprec}")
        relative_error = get_relative_error(mpfr, vprec)
        s = get_significant_digits(relative_error)

        vprec_range = get_var_env_int("VERIFICARLO_VPREC_RANGE")

        if not math.isinf(s) and (math.ceil(s) < precision):
            float_type = os.getenv("VERIFICARLO_VPREC_TYPE")
            vprec_precision = os.getenv("VERIFICARLO_PRECISION")
            vprec_mode = os.getenv("VERIFICARLO_VPREC_MODE")
            op = os.getenv("VERIFICARLO_OP")

            print(
                f" * {float_type}: "
                f"MODE={vprec_mode} RANGE={vprec_range} "
                f"PRECISION={vprec_precision} OP={op}",
                file=sys.stderr,
                flush=True,
            )

            print(
                f" * relative error too high: "
                f"a={input_ab[0]} b={input_ab[1]} "
                f"mpfr={mpfr} vprec={vprec} "
                f"error={relative_error} ( {s} base=2 )",
                file=sys.stderr,
                flush=True,
            )

            print(
                f" * mpfr={mpfr.hex()} vprec={vprec.hex()}", file=sys.stderr, flush=True
            )

            if exit_at_error():
                exit(1)


if "__main__" == __name__:
    precision = get_var_env_int(PRECISION)
    mpfr_list = parse_file(mpfr_file)
    vprec_list = parse_file(vprec_file)
    input_list = parse_file2(input_file)

    compute_err(precision, mpfr_list, vprec_list, input_list)

    exit(0)
