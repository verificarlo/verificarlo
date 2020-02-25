#!/usr/bin/env python3

"""Python implementation of the --debug-binary output
   Allows a double checking
"""
from __future__ import absolute_import
from __future__ import division
import math
import warnings
import argparse
import numpy as np

FLOAT_TYPES = ["float", "double"]

def is_substring(string, token):
    return string.find(token) != -1

def is_negative(hex_float):
    return hex_float.startswith('-')

def get_first_1bit(value_str):
    first1 = value_str.find('1')
    return first1

def remove_trailing_0(value_str):
    last1 = value_str.rfind('1')
    if last1 == -1:
        return '0'
    
    return value_str[:last1+1]

def remove_leading_0(value_str):
    first1 = value_str.find('1')
    return value_str[first1:]

# Extract fields from c99 hex float representation
# [s]0xH.hhhhp[+-]e -> s,H,hhhh,e
def extract_fields(hex_float):
    # [-]0xh.hhhhp[+-]e -> [-],h.hhhhp[+-]e
    sign, value = hex_float.split('0x')
    # h.hhhhp[+-]e -> h,hhhhp[+-]e
    implicit_bit, mantissa_exponent = value.split(".")
    mantissa, exponent = mantissa_exponent.split('p')
    return sign, implicit_bit, exponent, mantissa

def hex_to_bin(hex_value, mantissa_size):
    length_hex = len(hex_value)
    length_mantissa = int(math.ceil(mantissa_size/4.0))
    bin_value = ""
    for x in hex_value:
        bin_value += "{0:04b}".format(int(x, 16))
    for i in range(length_mantissa, length_hex):
        bin_value += "{0:04b}".format(0)
    return bin_value


class Binary(object):

    def __init__(self, hex_float):
        self.implicit_bit = '1'
        self.sign = '-' if is_negative(hex_float) else '+'
        if is_substring(hex_float, 'nan'):
            # Always return a +nan
            # IEEE-754-2008: For arithmetic operations,
            #  this standard does not specify the sign bit of a NaN
            #  result, even when there is only one input NaN,
            #  or when the NaN is produced from an invalid operation.
            self.sign = '+'
            self.exponent = self.exponent_max - self.exponent_bias
            self.mantissa = 0x1
            self.string = "{sign}nan".format(sign=self.sign)
        elif is_substring(hex_float, 'inf'):
            self.exponent = self.exponent_max - self.exponent_bias
            self.mantissa = 0x0
            self.string = "{sign}inf".format(sign=self.sign)
        elif is_substring(hex_float, '0x0.0p'):
            self.exponent = 0
            self.mantissa = 0x0
            self.string = "{sign}0.0 x 2^0".format(sign=self.sign)
        elif self.is_denormal():
            _, implicit_bit, exponent, mantissa = extract_fields(hex_float)
            self.mantissa = hex_to_bin(mantissa, self.mantissa_size)
            # self.mantissa = remove_trailing_0(self.mantissa)
            if implicit_bit == '0':
                if args.normalize_denormal:
                    first1 = get_first_1bit(self.mantissa)
                    self.exponent = int(exponent) - first1 - 1
                    self.mantissa = '{0:0<{size}b}'.format(int(self.mantissa, 2),
                                                           size=self.mantissa_size)
                    self.mantissa = self.mantissa[1:]
                    self.implicit_bit = '1'
                else:
                    self.exponent = self.exponent_min
                    self.implicit_bit = '0'
            else: # implicit_bit == '1'
                if args.normalize_denormal:
                    self.exponent = int(exponent)
                    self.implicit_bit = '1'
                else:
                    # Add implicit bit
                    offset = self.exponent_min - int(exponent) - 1
                    self.mantissa = remove_trailing_0(self.mantissa)
                    self.mantissa = '1{m}'.format(m=self.mantissa)
                    self.mantissa = self.mantissa.rjust(len(self.mantissa)+offset, '0')
                    self.exponent = self.exponent_min
                    self.implicit_bit = '0'

            self.mantissa = remove_trailing_0(self.mantissa)
            self.string = "{sign}{i}.{mant} x 2^{exp}".format(sign=self.sign,
                                                              i=self.implicit_bit,
                                                              mant=self.mantissa,
                                                              exp=self.exponent)


        else:
            sign, implicit_bit, exponent, mantissa = extract_fields(hex_float)
            # print(sign,implicit_bit,exponent,mantissa)
            self.implicit_bit = int(implicit_bit)
            self.mantissa = hex_to_bin(mantissa, self.mantissa_size)
            self.mantissa = remove_trailing_0(self.mantissa)
            self.exponent = int(exponent)
            self.string = "{sign}{i}.{mant} x 2^{exp}".format(sign=self.sign,
                                                              i=self.implicit_bit,
                                                              mant=self.mantissa,
                                                              exp=self.exponent)

class Binary32(Binary):

    sign_size = 1
    exponent_size = 8
    mantissa_size = 23
    exponent_bias = 0x7f
    exponent_max = 0xff
    exponent_min = -0x7e
    mantissa_offset = sign_size + exponent_size
    smallest_normal = float.fromhex("0x1.0p-126")

    def __init__(self, float_str):
        if isinstance(float_str, np.float32):
            self.value = float(float_str)
            super(Binary32, self).__init__(float(float_str).hex())
        else:
            self.value = float.fromhex(float_str)
            super(Binary32, self).__init__(float_str)

    def __str__(self):
        fmt = "Binary32 [sign : {sign}; \
        exponent: {exponent}; \
        mantissa: {mantissa}]".format(sign=self.sign,
                                      exponent=self.exponent,
                                      mantissa=self.mantissa)
        return fmt

    def is_denormal(self):
        return abs(self.value) < self.smallest_normal

class Binary64(Binary):

    sign_size = 1
    exponent_size = 11
    mantissa_size = 52
    exponent_bias = 0x3ff
    exponent_max = 0x7ff
    exponent_min = -0x3fe
    mantissa_offset = sign_size + exponent_size
    smallest_normal = float.fromhex("0x1.0p-1022")

    def __init__(self, flt):
        if type(flt) == float:
            self.value = float(flt)
            super(Binary64, self).__init__(flt.hex())
        else:
            self.value = float.fromhex(flt)
            super(Binary64, self).__init__(flt)

    def __str__(self):
        fmt = "Binary64 [sign : {sign}; \
        exponent: {exponent}; \
        mantissa: {mantissa}]".format(sign=self.sign,
                                      exponent=self.exponent,
                                      mantissa=self.mantissa)
        return fmt

    def is_denormal(self):
        return abs(self.value) < self.smallest_normal


def parse_line(line):
    x, y, op = line.strip().split()
    return x, y, op

# takes a float in c99 hex format [-]0xh.hhhhp[+-]e
def get_binary(float_type, hex_float):
    if float_type == "double":
        b = Binary64(hex_float)
    elif float_type == "float":
        b = Binary32(hex_float)
    else:
        raise Exception('Unknow format {type}'.format(type=float_type))

    return b.string

def get_float_operation(float_type, op_str):
    if float_type == 'double':
        flt = float
    elif float_type == 'float':
        flt = np.float32
    else:
        raise Exception("Unknown type {ty}".format(ty=float_type))

    if op_str == "+":
        return lambda args: flt(args[0])+flt(args[1])
    elif op_str == "-":
        return lambda args: flt(args[0])-flt(args[1])
    elif op_str == "*":
        return lambda args: flt(args[0])*flt(args[1])
    elif op_str == "/":
        return lambda args: flt(args[0])/flt(args[1])
    else:
        raise Exception("Unknown operation {op}".format(op=op_str))

if "__main__" == __name__:

    parser = argparse.ArgumentParser(description="Generate output as --debug-binary mode of libinterflop_ieee.so")
    parser.add_argument('-t', '--float-type', choices=FLOAT_TYPES, required=True, help='float type')
    parser.add_argument('-f', '--filename', required=True, help='filename with values to print')
    parser.add_argument('--normalize-denormal', action='store_true', help='print denormals in a normalized way')

    args, other = parser.parse_known_args()

    float_type = args.float_type
    filename = args.filename

    fi = open(filename)

    for line in fi:
        if len(line.split()) == 3:
            x_str, y_str, op_str = parse_line(line)
            op = get_float_operation(float_type, op_str)
            x_flt = float.fromhex(x_str)
            y_flt = float.fromhex(y_str)

            with warnings.catch_warnings():
                warnings.filterwarnings('ignore', r'overflow encountered in float_scalars')
                res = op((x_flt, y_flt))

            x_bin = get_binary(float_type, x_str)
            y_bin = get_binary(float_type, y_str)
            res_bin = get_binary(float_type, res)

            print('{x} {op} '.format(x=x_bin, op=op_str))
            print('{y} -> '.format(y=y_bin))
            print('{res}\n'.format(res=res_bin))
