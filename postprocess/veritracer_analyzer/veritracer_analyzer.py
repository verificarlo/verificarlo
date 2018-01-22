#!/usr/bin/python

import sys
import argparse
import os
import csv
import math
import mmap
import struct
import timeit
import crange_tracer_stat
from functools import partial
from collections import deque,namedtuple

Ko = 1024
Mo = 1024**2
Go = 1024**3

NS_MAX = {4 : 7, 8 : 16}

csv_header = ["hash", "type", "time",
              "max","min","median","mean","std",
              "number-significant-digit"]

offset = 0
local_offset = 0
size_to_read = 10*Mo
    
def compute_nb_significant_digits(mean, std, size_bytes):

    if mean != 0.0 and std != 0.0:
        return -math.log10(abs(std/mean))
    elif mean != 0.0 and std == 0.0:
        return NS_MAX[size_bytes]
    else:
        return 0

def parse_file(filename):
    
    try:
        f = open(filename, 'r+b')
    except IOError as e:
        print "Could not open " + filename + " " + str(e)
        return

    mm = mmap.mmap(f.fileno(), 0)    
    mm.seek(offset)
    
    list_values = deque()
    append = list_values.append
    
    size_file = mm.size()

    try:
        while size_to_read + offset >  mm.tell() :
            sizeVb = mm.read(4) 
            sizeVl = struct.unpack('I',sizeVb)            
            sizeV = sizeVl[0]
            values = []
            if sizeV == 4:
                strb = mm.read(28)
                values = struct.unpack('LPLf', strb)
            elif sizeV == 8:
                strb = mm.read(32)
                values = struct.unpack('LPLd', strb)
            else:
                print "Unknow size : " + str(sizeV)
                exit(1)
            append(sizeVl + values)

    except Exception as e:
        if size_to_read + offset <  mm.tell():
            print 'Parse_file ERRROR ' + str(e)

    global local_offset
    local_offset = mm.tell()
    mm.close()
    return list_values

def parse_file_factory(filename):
    sizefileB = os.stat(filename).st_size
    f = open(filename, 'r+b')

def transpose(list_exp):
    return map(lambda exp : zip(*exp),zip(*list_exp))    

def checkEqualItem(lst):
    return not lst or lst.count(lst[0]) == len(lst)

def parse_directory(list_files):
    global offset, local_offset
        
    list_exp = map(parse_file, list_files)
    list_exp = filter(lambda x : x != None, list_exp)   
    offset = local_offset

    return transpose(list_exp)

def get_files(args):
    
    filename = args.filename
    list_dir = deque(os.listdir('.'))
    list_directories = filter(os.path.isdir, list_dir)
    list_files = map(lambda d: os.path.join(d,filename), list_directories)
    return filter(os.path.isfile, list_files)
    
# Each value is a tuple 
def compute_error(functions, list_values):
        
    list_value_T     = list_values
    size_bytes_value = list_value_T[0][0]
    timestamp        = list_value_T[1][0]
    ptr              = list_value_T[2][0]
    hash_locInfo     = list_value_T[3][0]
    
    list_FP = functions.cArrayBuild(*list_value_T[4])
    meand = functions.mean(list_FP)
    stdd = functions.std(meand,list_FP)
    mediand = functions.median(list_FP)
    maxdv = functions.max(list_FP)
    mindv = functions.min(list_FP)
        
    nb_sigdgt = compute_nb_significant_digits(meand, stdd, size_bytes_value)
        
    d_errors = {
        'hash'                     : hash_locInfo,
        'type'                     : size_bytes_value,
        'time'                     : timestamp,   
        'max'                      : maxdv,
        'min'                      : mindv,
        'mean'                     : meand,
        'median'                   : mediand,
        'std'                      : stdd,
        'number-significant-digit' : nb_sigdgt
    }

    return d_errors
    
def parse_values(statsFunctions, list_exp):
    
    list_nb_lines = map(len,list_exp)
    assert(checkEqualItem(list_nb_lines))

    return map(lambda x : compute_error(statsFunctions, x), list_exp)


def open_csv(filename):
    output = filename
    fo = open(output, 'wb')
    csv_writer = csv.DictWriter(fo, fieldnames=csv_header)
    csv_writer.writeheader()
    return csv_writer

def write_csv(filename, list_errors):
    csv_writer = open_csv(filename)
    csv_writer.writerows(list_errors)

def main():
    parser = argparse.ArgumentParser(
        description="Gathering values from several veritracer executions", add_help=True)
    parser.add_argument('-f','--filename', type=str, default="veritracer.dat")
    parser.add_argument('-o','--output', type=str, default="veritracer.csv")
    parser.add_argument('-N','--read-bytes', type=int, default=0,
                        help='Read N first bytes')
    parser.add_argument('--verbose', action="store_true",
                        help="Verbose mode")
    
    args = parser.parse_args()
    
    sizefile = os.path.getsize('1/'+args.filename)

    list_files = get_files(args)
    statsFunctions = crange_tracer_stat.load_functions(len(list_files))

    output_file = args.output
    
    n_ = sizefile / size_to_read + 1
    for i in xrange(n_):
        # Parse dir
        list_exp = parse_directory(list_files)

        # Parse values
        list_errors = parse_values(statsFunctions, list_exp)

        # Write csv
        output_file_i = output_file + "_" + str(i)
        write_csv(output_file_i, list_errors)
                                
if '__main__' == __name__:
    
    main()
