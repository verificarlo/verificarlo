#!/usr/bin/env python

import sys
import argparse
import os
import csv
import mmap
import struct
from collections import deque, namedtuple
import veritracer_backtrace as vtr_backtrace
import veritracer_math as vtr_math

Ko = 1024
Mo = 1024**2
Go = 1024**3

csv_header = ["hash","type","time","max","min","median","mean","std","significant_digit_number"]

offset = 0
local_offset = 0
size_to_read = 10*Mo

ValuesLine = namedtuple('ValuesLine',['format','time','hash','address','value'])
StatsLine = namedtuple('StatsLine',['hash','type','time',
                                    'max','min','mean',
                                    'median','std','significant_digit_number'])

def init_module(subparsers, veritracer_plugins):
    veritracer_plugins["analyzer"] = run
    analyzer_parser = subparsers.add_parser("analyzer", help="Gathering values from several veritracer executions")
    analyzer_parser.add_argument('-f','--filename', type=str, default="veritracer.dat", metavar='',
                                 help="filename of the trace to gather")
    analyzer_parser.add_argument('-o','--output', type=str, default="veritracer", metavar='',
                                 help='output filename')
    analyzer_parser.add_argument('-N','--read-bytes', type=int, default=0, metavar='',
                                 help='read the N first bytes')
    analyzer_parser.add_argument('--prefix-dir', type=str, default="", metavar='',
                                 help='prefix of the directory to analyze')
    analyzer_parser.add_argument('--backtrace-filename', type=str, default="backtrace.dat", metavar='',
                                 help='filename of the backtrace to use')
    analyzer_parser.add_argument('--verbose', action="store_true",
                                 help="verbose mode")
    analyzer_parser.add_argument('--format', action='store', choices=['binary','text'], default='binary',
                                 help='veritracer.dat encoding format')

    
# Return size in bytes associated to the given format
def parse_fmt_text(fmt):
    if fmt.find('32'):
        return 4
    elif fmt.find('64'):
        return 8
    else:
        print "Error: unknown format %s" % fmt
        exit(1)

# Args order fmt,time,hash,ptr,val
def parse_raw_line_text(line):

    line_split = line.split()
    rawvalue = ValuesLine(format=line_split[0],
                          time=line_split[1],
                          hash=line_split[2],
                          address=line_split[3],
                          value=line_split[4])
    
    size  = parse_fmt_text(rawvalue.format)
    ptr   = "0x0" if rawvalue.address == "(nil)" else rawvalue.address
    time  = int(rawvalue.time)
    hashv = int(rawvalue.hash)        
    val = vtr_math.hexa_to_fp(rawvalue.value)

    value = ValuesLine(format=size,
                       time=time,
                       address=ptr,
                       hash=hashv,
                       value=val) 
    return value

# Args order fmt,time,hash,ptr,val
def parse_raw_line_binary(line):
    # print line
    rawvalue = ValuesLine(format=line[0],
                          time=line[1],
                          hash=line[3],
                          address=line[2],
                          value=line[4])
    
    size  = rawvalue.format
    ptr   = "0x0" if rawvalue.address == "(nil)" else rawvalue.address
    time  = int(rawvalue.time)
    hashv = int(rawvalue.hash)        
    val   = rawvalue.value

    value = ValuesLine(format=size,
                       time=time,
                       address=ptr,
                       hash=hashv,
                       value=val)
    
    return value

def parse_text_file(filename):
    try:
        fi = open(filename, 'r+b')
    except IOError as e:
        print "Could not open " + filename + " " + str(e)
        return

    if os.path.getsize(filename) == 0:
        print "Error: %s is empty" % filename
        exit(1)

    values_list = [parse_raw_line_text(line) for line in fi]
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
            values_line = parse_raw_line_binary(sizeVl+values)
            append(values_line)

    except Exception as e:
        if size_to_read + offset <  mm.tell():
            print 'Parse_file ERRROR ' + str(e)
        # else:
        #     print e
            # exit(1)
            
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

def parse_exp(exp):
    format = map(lambda valueLine : valueLine.format, exp)
    time = map(lambda valueLine : valueLine.time, exp)
    address = map(lambda valueLine : valueLine.address, exp)
    hashv = map(lambda valueLine : valueLine.hash, exp)
    value = map(lambda valueLine : valueLine.value, exp)

    valueline = ValuesLine(format=format[0],
                          time=time[0],
                          address=address[0],
                          hash=hashv[0],
                          value=value)

    return valueline

def parse_directory(args, list_files):
    global offset, local_offset
        
    list_exp = map(lambda file : parse_file(args, file), list_files)
    list_exp = filter(lambda x : x != None, list_exp)   
    offset = local_offset

    value_exp = [parse_exp(exp) for exp in zip(*list_exp)]
    return value_exp
    return transpose(list_exp)

def get_files(args):
    
    filename = args.filename
    list_dir = deque(os.listdir('.'))
    list_directories = filter(os.path.isdir, list_dir)
    list_files = map(lambda d: os.path.join(d,filename), list_directories)
    return filter(os.path.isfile, list_files)

# Each value is a tuple 
def compute_stats(values_list):

    sizeof_value = values_list.format
    time         = values_list.time
    ptr          = values_list.address
    hashv         = values_list.hash
    list_FP      = values_list.value

    mean_   = vtr_math.mean(list_FP)
    std_    = vtr_math.std(mean_, list_FP)
    median_ = vtr_math.median(list_FP)
    max_    = max(list_FP)
    min_    = min(list_FP)    
    sdn_    = vtr_math.sdn(mean_, std_, sizeof_value)

    stats = StatsLine(hash=hashv,
                      type=sizeof_value,
                      time=time,   
                      max=max_,
                      min=min_,
                      mean=mean_,
                      median=median_,
                      std=std_,
                      significant_digit_number=sdn_)

    return stats
    
def parse_values(list_exp):    
    return map(lambda x : compute_stats(x), list_exp)

def open_csv(filename):
    output = filename
    fo = open(output, 'wb')
    csv_writer = csv.DictWriter(fo, fieldnames=csv_header)
    csv_writer.writeheader()
    return csv_writer

def write_csv(filename, list_errors):
    csv_writer = open_csv(filename)
    for row in list_errors:
        csv_writer.writerow(row._asdict())
        
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
                                
# if '__main__' == __name__:
    
#     main()


def run(args):
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

    return True
