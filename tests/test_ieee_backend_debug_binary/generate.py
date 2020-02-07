#!/usr/bin/env python3

"""Python implementation of the --debug-binary output
   Allows a double checking
"""

import sys
import numpy as np
import struct
import math
import warnings

float_types = ["float", "double"]

def is_substring(string, token):
    return string.find(token) != -1

def is_negative(hex_float):
    return hex_float.startswith('-')

# Returns the position of the first 1-bit
# of a number
def get_first_1bit(x):
    if type(x) == str:
        x = int('1'+x,16)
    elif type(x) == int:
        x = int('1'+str(x),16)
    binx = bin(x).replace('0b','')
    return binx.find('1',1)-1

# Remove the first 1-bit of a number
def remove_first_1bit(x):
    if type(x) == str:
        x = int(x)
    sign_mask = 0x80000000
    sign = sign_mask & x
    xint = sign_mask | x
    binx = bin(xint)
    nb_of_replacement = 1 if sign else 2
    binx = binx.replace('1','0', nb_of_replacement)
    return int(binx,2)

def remove_trailing_0(x_int):
    if (x_int % 2 == 1) or (x_int == 0):
        return x_int
    else:
        binx = bin(x_int).replace('0b','')
        first1 = binx.find('1')
        last1 = binx.rfind('1')
        if first1 == last1:
            return 1
        else:
            binx = binx[:last1+1]
            return int(binx,2)

# Extract fields from c99 hex float representation
# [s]0xH.hhhhp[+-]e -> s,H,hhhh,e
def extract_fields(hex_float):
    # [-]0xh.hhhhp[+-]e -> [-],h.hhhhp[+-]e
    sign,value = hex_float.split('0x')
    # h.hhhhp[+-]e -> h,hhhhp[+-]e
    implicit_bit,mantissa_exponent = value.split(".")
    mantissa,exponent = mantissa_exponent.split('p')
    return sign,implicit_bit,exponent,mantissa

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
        else:
            sign,implicit_bit,exponent,mantissa = extract_fields(hex_float)
            self.implicit_bit = int(implicit_bit)
            self.mantissa = remove_trailing_0(int(mantissa, 16))
            self.exponent = int(exponent)

            # Denormal case, we normalize the value
            if self.implicit_bit == 0:
                self.mantissa_offset = get_first_1bit(mantissa)
                self.mantissa = remove_first_1bit(mantissa)
                self.mantissa = remove_trailing_0(self.mantissa)
                self.exponent = self.exponent_min - self.mantissa_offset - 1
                self.implicit_bit = "1"

            mantissa_bin = bin(self.mantissa).replace('0b','')
            self.string = "{sign}{i}.{mant} x 2^{exp}".format(sign=self.sign,
                                                              i=self.implicit_bit,
                                                              mant=mantissa_bin,
                                                              exp=self.exponent)

class Binary32(Binary):

    sign_size = 1
    exponent_size = 8
    mantissa_size = 23
    exponent_bias = 0x7f
    exponent_max = 0xff
    exponent_min = -0x7e
    mantissa_offset = sign_size + exponent_size

    def __init__(self, float_str):
        if type(float_str) == np.float32:
            super(Binary32, self).__init__(float(float_str).hex())
        else:
            super(Binary32, self).__init__(float_str)

    def __str__(self):
        fmt="Binary32 [sign : {sign}; \
        exponent: {exponent}; \
        mantissa: {mantissa}]".format(sign=self.sign,
                                      exponent=self.exponent,
                                      mantissa=self.mantissa)
        return fmt

class Binary64(Binary):

    sign_size = 1
    exponent_size = 11
    mantissa_size = 52
    exponent_bias = 0x3ff
    exponent_max = 0x7ff
    exponent_min = -0x3fe
    mantissa_offset = sign_size + exponent_size

    def __init__(self, flt):
        if type(flt) == float:
            hex_float = flt.hex()
            super(Binary64, self).__init__(hex_float)
        else:
            super(Binary64, self).__init__(flt)

    def __str__(self):
        fmt="Binary64 [sign : {sign}; \
        exponent: {exponent}; \
        mantissa: {mantissa}]".format(sign=self.sign,
                                      exponent=self.exponent,
                                      mantissa=self.mantissa)
        return fmt

def parse_line(line):
    x,y,op = line.strip().split()
    return x,y,op

# takes a float in c99 hex format [-]0xh.hhhhp[+-]e
def get_binary(float_type, hex_float):
    if float_type == "double":
        b = Binary64(hex_float)
    elif float_type == "float":
        b = Binary32(hex_float)
    else:
        exception('Unknow format {type}'.format(type=float_type))

    return b.string


def check_float_type(float_type):
    if not float_type.lower() in float_types:
        print("Unknown float type : {float_type}".format(float_type=float_type))
        print("Available float types : {flt}".format(flt=float_types))
        exit(1)

def get_float_operation(float_type, op_str):
    if float_type == 'double':
        flt = float
    elif float_type == 'float':
        flt = np.float32
    else:
        raise(Exception("Unknown operation {op}".format(op=op_str)))

    if op_str == "+":
        return lambda args:flt(args[0])+flt(args[1])
    elif op_str == "-":
        return lambda args:flt(args[0])-flt(args[1])
    elif op_str == "*":
        return lambda args:flt(args[0])*flt(args[1])
    elif op_str == "/":
        return lambda args:flt(args[0])/flt(args[1])

if "__main__" == __name__:

    if (len(sys.argv) != 3):
        print("usage <float_type> <filename>")
        print('<float_type>: {flt}'.format(flt=float_types))
        exit(1)

    float_type = sys.argv[1]
    filename = sys.argv[2]

    check_float_type(float_type)

    fi = open(filename)

    for line in fi:
        if len(line.split()) == 3:
            x_str,y_str,op_str = parse_line(line)
            op = get_float_operation(float_type, op_str)
            x_flt = float.fromhex(x_str)
            y_flt = float.fromhex(y_str)

            with warnings.catch_warnings():
                warnings.filterwarnings('ignore', r'overflow encountered in float_scalars')
                res = op((x_flt,y_flt))

            x_bin = get_binary(float_type, x_str)
            y_bin = get_binary(float_type, y_str)
            res_bin = get_binary(float_type, res)

            print('{x} {op} '.format(x=x_bin, op=op_str))
            print('{y} -> '.format(y=y_bin))
            print('{res}\n'.format(res=res_bin))
