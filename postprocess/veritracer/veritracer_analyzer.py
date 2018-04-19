#!/usr/bin/python

import sys
import argparse
import os
import csv
import mmap
import struct
from collections import deque
import veritracer_backtrace as vtr_backtrace
import veritracer_math as vtr_math

Ko = 1024
Mo = 1024**2
Go = 1024**3

csv_header = ["hash","type","time","max","min","median","mean","std","significant-digit-number"]

offset = 0
local_offset = 0
size_to_read = 10*Mo

# Return size in bytes associated to the given format
def parse_fmt_text(fmt):
    if fmt.find('32'):
        return 4
    elif fmt.find('64'):
        return 8
    else:
        print "Error: unknown format %s" % fmt
        exit(1)
    
def parse_text_file(filename):
    try:
        fi = open(filename, 'r+b')
    except IOError as e:
        print "Could not open " + filename + " " + str(e)
        return

    if os.path.getsize(filename) == 0:
        print "Error: %s is empty" % filename
        exit(1)

    values_list = deque()
    append = values_list.append

    for line in fi:
        [fmt, time, hashv_, ptr_, value_] = line.split()
        size = parse_fmt_text(fmt)
        ptr = 0 if ptr_ == "(nil)" else int(ptr_,16)
        time = int(time)
        hashv = int(hashv_)        
        value = vtr_math.hexa_to_fp(value_)
        append([size, time, ptr, hashv, value])

    return values_list
        
def parse_binary_file(filename):

    try:
        f = open(filename, 'r+b')
    except IOError as e:
        print "Could not open " + filename + " " + str(e)
        return

    if os.path.getsize(filename) == 0:
        print "Error: %s is empty" % filename
        exit(1)

    mm = mmap.mmap(f.fileno(), 0)    
    mm.seek(offset)
    
    values_list = deque()
    append = values_list.append
    
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
    return values_list

def parse_file(args, filename):
    if args.format == "binary":
        return parse_binary_file(filename)
    else:
        return parse_text_file(filename)

def transpose(list_exp):
    return map(lambda exp : zip(*exp),zip(*list_exp))    

def checkEqualItem(lst):
    return not lst or lst.count(lst[0]) == len(lst)

def parse_directory(args, list_files):
    global offset, local_offset
        
    list_exp = map(lambda f : parse_file(args,f), list_files)
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
def compute_stats(values_list):
        
    sizeof_value = values_list[0][0]
    timestamp        = values_list[1][0]
    ptr              = values_list[2][0]
    hash_locInfo     = values_list[3][0]
    
    list_FP = values_list[4][:]

    mean_   = vtr_math.mean(list_FP)
    std_    = vtr_math.std(mean_, list_FP)
    median_ = vtr_math.median(list_FP)
    max_    = max(list_FP)
    min_    = min(list_FP)
    
    SDN = vtr_math.sdn(mean_, std_, sizeof_value)
        
    d_errors = {
        'hash'                     : hash_locInfo,
        'type'                     : sizeof_value,
        'time'                     : timestamp,   
        'max'                      : max_,
        'min'                      : min_,
        'mean'                     : mean_,
        'median'                   : median_,
        'std'                      : std_,
        'significant-digit-number' : SDN
    }

    return d_errors
    
def parse_values(list_exp):
    
    list_nb_lines = map(len,list_exp)
    assert(checkEqualItem(list_nb_lines))

    return map(lambda x : compute_stats(x), list_exp)

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
    parser.add_argument('-o','--output', type=str, default="veritracer")
    parser.add_argument('-N','--read-bytes', type=int, default=0,
                        help='Read N first bytes')
    parser.add_argument('--prefix-dir', type=str, default="",
                        help='directories prefix to analyze')
    parser.add_argument('--backtrace-filename', type=str, default="backtrace.dat",
                        help='backtrace filename to use')
    parser.add_argument('--verbose', action="store_true",
                        help="Verbose mode")
    parser.add_argument('--format', action='store', choices=['binary','text'], default='binary',
                        help='veritracer.dat encoding format')
    
    args = parser.parse_args()

    # Dict which maps bt_name to associated directories
    dict_bt_files = vtr_backtrace.partition_samples(args)
    
    for bt_name, dir_list in dict_bt_files.iteritems():

        if args.verbose:
            print bt_name, dir_list
        
        global offset, local_offset
        offset = 0
        local_offset = 0
        
        list_files = map(lambda dir_ : dir_ + os.sep + args.filename, dir_list)
        filesize =  os.path.getsize(list_files[0])
        output_file = args.output + "." + bt_name

        n_ = filesize / size_to_read + 1
        for i in xrange(n_):
            # Parse dir
            list_exp = parse_directory(args, list_files)

            # Parse values
            # list_errors = parse_values(statsFunctions, list_exp)
            list_errors = parse_values(list_exp)

            # Write csv
            output_file_i = output_file + "." + str(i)
            write_csv(output_file_i, list_errors)
                                
if '__main__' == __name__:
    
    main()
