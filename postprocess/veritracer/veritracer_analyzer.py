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
import re
from functools import partial
from collections import deque,namedtuple

Ko = 1024
Mo = 1024**2
Go = 1024**3

NS_MAX = {4 : 7, 8 : 16}

csv_header = ["hash", "type", "time",
              "max","min","median","mean","std",
              "significant-digit-number"]

offset = 0
local_offset = 0
size_to_read = 10*Mo

BacktraceLine = namedtuple('BacktraceLine', ['binary_path','function','address'])
backtrace_separator = "###"
backtrace_dirname = ".backtrace"
backtrace_map_filename = "backtraces.map"
backtrace_map_file = None

def compute_nb_significant_digits(mean, std, size_bytes):

    if std != 0.0:
        if mean != 0.0:
            return -math.log10(abs(std/mean))
        else:
            return 0
    else: # std == 0.0
        return NS_MAX[size_bytes]                    

def parse_file(args, filename):
    if args.format == "binary":
        return parse_binary_file(filename)
    else:
        return parse_text_file(filename)

# Translate from C99 hexa representation to FP
# 0x+/-1.mp+e, m mantissa and e exponent
def hexa_to_fp(hexafp):
    return float.fromhex(hexafp)

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
        f = open(filename, 'r+b')
    except IOError as e:
        print "Could not open " + filename + " " + str(e)
        return

    if os.path.getsize(filename) == 0:
        print "Error: %s is empty" % filename
        exit(1)

    list_values = deque()
    append = list_values.append

    for line in f:
        [fmt, time, hashv_, ptr_, value_] = line.split()
        size = parse_fmt_text(fmt)
        ptr = 0 if ptr_ == "(nil)" else int(ptr_,16)
        time = int(time)
        hashv = int(hashv_)        
        value = hexa_to_fp(value_)
        append([size, time, ptr, hashv, value])

    return list_values
        
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

def mean(list_FP):
    return math.fsum(list_FP) / len(list_FP)

def std(mean, x):
    ln = len(x)
    if ln < 2:
        return 0.0
    n = 0
    m = mean
    M2 = 0.0
    for xi in x:
        n += 1
        delta = xi - m
        m += delta / n
        M2 += delta*(xi - m)
    return math.sqrt(M2/(ln-1))

def median(x):
    ln = len(x)
    if ln == 1:
        return x[0]
    else:
        s = sorted(x)    
        s_odd = s[(ln-1)/2]
        if ln % 2 == 0:
            return (s_odd+s[ln/2])/2
        else:
            return s_odd

# Each value is a tuple 
# def compute_error(functions, list_values):
def compute_error(list_values):
        
    list_value_T     = list_values
    size_bytes_value = list_value_T[0][0]
    timestamp        = list_value_T[1][0]
    ptr              = list_value_T[2][0]
    hash_locInfo     = list_value_T[3][0]
    
    list_FP = list_value_T[4][:]
    # list_FP = functions.cArrayBuild(*list_value_T[4])
    # print ",".join(map(str,list_FP))
    # meand = functions.mean(list_FP)
    # stdd = functions.std(meand,list_FP)
    # print "std",stdd
    # mediand = functions.median(list_FP)
    # maxdv = functions.max(list_FP)
    # mindv = functions.min(list_FP)

    meand = mean(list_FP)
    stdd = std(meand, list_FP)
    mediand = median(list_FP)
    maxdv = max(list_FP)
    mindv = min(list_FP)
    
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
        'significant-digit-number' : nb_sigdgt
    }

    return d_errors
    
def parse_values(list_exp):
# def parse_values(statsFunctions, list_exp):
    
    list_nb_lines = map(len,list_exp)
    assert(checkEqualItem(list_nb_lines))

    # return map(lambda x : compute_error(statsFunctions, x), list_exp)
    return map(lambda x : compute_error(x), list_exp)

def open_csv(filename):
    output = filename
    fo = open(output, 'wb')
    csv_writer = csv.DictWriter(fo, fieldnames=csv_header)
    csv_writer.writeheader()
    return csv_writer

def write_csv(filename, list_errors):
    csv_writer = open_csv(filename)
    csv_writer.writerows(list_errors)

def get_dir_list(args, getBacktraceDir=False):
    listdir = os.listdir(".")
    dirs = filter(os.path.isdir, listdir)
    dirs_with_prefix = filter(lambda d : d.find(args.prefix_dir) != -1, dirs)
    dirs_with_veritracer_file = filter(lambda d : os.path.isfile(d+os.sep+args.filename), dirs_with_prefix)
    dirs_with_backtrace_file = filter(lambda d : os.path.isfile(d+os.sep+args.backtrace_filename), dirs_with_prefix)
    
    return dirs_with_backtrace_file if getBacktraceDir else dirs_with_veritracer_file

def get_filesize(args):
    listdir = os.listdir(".")
    dirs = filter(os.path.isdir, listdir)
    dirs_with_prefix = filter(lambda d : d.find(args.prefix_dir) != -1, dirs)
    dirs_with_veritracer_file = filter(lambda d : os.path.isfile(d+os.sep+args.filename), dirs_with_prefix)
    # dirs_with_backtrace_file = filter(lambda d : os.path.isfile(d+os.sep+args.backtrace), dirs_with_prefix)
    filessize = map(lambda d : os.path.getsize(d + os.sep + args.filename), dirs_with_veritracer_file)

    return filessize[0]

def get_hash_backtrace(backtrace):

    backtrace_string = "".join(backtrace)
    return hash(backtrace_string)

def get_hash_backtrace_list(backtrace_list):

    backtrace_string = "".join(["".join(backtrace) for backtrace in backtrace_list])
    return hash(backtrace_string)

# backtrace line format
# ../../test(get_backtrace+0x10)[0x404750]
# <binary_path>(<function>+<offset>)[address]
def parse_backtrace_line(line):
    try:
        binary_path_ = re.findall('^.*(?=\()',line)        
        if (len(binary_path_) == 0):
            binary_path_ = re.findall('^.*(?=\[)',line)
        binary_path = binary_path_[0] if len(binary_path_) != 0 else ""
        function_ = re.findall('(?<=\().*(?=\))',line)
        function = function_[0] if len(function_) != 0 else ""
        address_ = re.findall('(?<=\[).*(?=\])',line)
        address = address_[0] if len(address_) != 0 else ""
    except Exception as e:
        print "Error while parsing backtrace line: %s" % line
        print e
        exit(1)
        
    btline = BacktraceLine(binary_path, function, address)
    return btline
    
def parse_backtrace(backtrace_filename):

    backtrace_file = open(backtrace_filename, "r")

    backtrace = []
    backtrace_list = []
    for line in backtrace_file:
        if line.find(backtrace_separator) != -1:
            backtrace.append(line)
            backtrace_list.append(backtrace)
            backtrace = []
        else:
            btline = parse_backtrace_line(line)
            backtrace.append(btline)            

    return backtrace_list

def dump_backtrace_list(bt_filename, backtrace_list):

    bt_file = open(bt_filename, "w")

    for backtrace in backtrace_list:
        for bt_line in backtrace:
            if type(bt_line) == BacktraceLine:
                str_to_write = bt_line.binary_path + ":" + bt_line.function + "\n"
            else: # backtrace_separator + hash_value
                str_to_write = bt_line 
            bt_file.write(str_to_write)
        # bt_file.write(backtrace_separator+"\n")

def add_backtrace_mapping(backtrace_list_name, dirs_list):
    global backtrace_map_file
    if backtrace_map_file == None:
        backtrace_map_file = open(backtrace_dirname + os.sep + backtrace_map_filename, "w")
    backtrace_map_file.write(backtrace_list_name + ":")
    str_to_write = ",".join(dirs_list)
    backtrace_map_file.write(str_to_write+"\n")

def is_same_backtrace(bt1,bt2):

    for bt_line1, bt_line2 in zip(bt1, bt2):
        if type(bt_line1) == BacktraceLine:
            if bt_line1.binary_path != bt_line2.binary_path or bt_line1.function != bt_line2.function:
                return False
        elif type(bt_line1) == str:
            return bt_line1 == bt_line2
    return True

def is_same_backtrace_list(bt_list1, bt_list2):

    for bt1, bt2 in zip(bt_list1, bt_list2):
        if not is_same_backtrace(bt1, bt2):
            return False
    return True

# We partition the samples according to their backtrace list
def partition_samples(args):
    
    # Directories list containing a veritracer.dat file
    dir_list = get_dir_list(args)

    # Dict which map a sample/directory to its backtrace
    dir_bt_dict = {}
    for dir_ in dir_list:
        backtrace_list = parse_backtrace(dir_ + os.sep + args.backtrace_filename)
        dir_bt_dict[dir_] = backtrace_list
        
    # Create directory for gathering backtrace information
    if not os.path.exists(backtrace_dirname):
        os.makedirs(backtrace_dirname)

    # we order sample by theire backtrace list size
    # it is useless to compare backtraces that have not the same size
    dict_vi = dir_bt_dict.viewitems()
    i = 0

    dict_to_return = {}
    
    while dir_bt_dict != {}:
        (dir_,bt_list) = dir_bt_dict.popitem()
        dirs_list = [dir_]
        list_dirs_with_same_bt_size = filter(
            lambda (_,btl) : len(btl) == len(bt_list)
            , dict_vi)
        # print map(lambda (d,b) : (d,len(b)), dict_vi)
        # print list_dirs_with_same_bt_size
        for dirs_,bt_list_to_cmp in list_dirs_with_same_bt_size:
            if is_same_backtrace_list(bt_list, bt_list_to_cmp):
                dirs_list.append(dirs_)
                dir_bt_dict.pop(dirs_)

        backtrace_list_name = str(i) + ".bt"
        add_backtrace_mapping(backtrace_list_name, dirs_list)
        backtrace_list_filename = backtrace_dirname + os.sep + backtrace_list_name
        dump_backtrace_list(backtrace_list_filename, bt_list)

        dict_to_return[backtrace_list_name] = dirs_list
        
        i += 1

    return dict_to_return

def check_line(line):
    return len(set(line)) == 1

# Partion the list according
# to the bt size
def split_bt_list(bt_list, counter):
    # get the different lines
    diff_lines = set(map(lambda bt:bt[counter], bt_list))

    bt_list_splitted = []
    for line in diff_lines:
        bt_list_filtered = filter(lambda bt: bt[counter] == line, bt_list)
        bt_list_splitted.append(map(lambda bt : bt[counter+1:], bt_list_filtered))

    return filter(lambda bt : bt != [],bt_list_splitted)
                            
def check_divergence(bt_list, counter):
    counter_local = 0
    print bt_list
    minlen = min(map(len,bt_list))
    maxlen = max(map(len,bt_list))
    bt_tree = []
    bt_list_splitted = []
    for i in range(minlen):
        line = map(lambda bt : bt[i], bt_list)
        is_good_line = check_line(line)
        print line,is_good_line
        if  not is_good_line:
            bt_tree.append((set(line),counter))
            bt_list_splitted = split_bt_list(bt_list, counter_local)
            bt_tree.extend(map(lambda bt:check_divergence(bt,counter+1), bt_list_splitted))
            break
        counter_local += 1
        counter += 1
    if minlen != maxlen:
        bt_tree.append((set(line),counter))
        bt_tree.extend(map(lambda bt:check_divergence(bt,counter+1), bt_list_splitted))
    return filter(lambda bt : bt != [], bt_tree)


    
def main():
    parser = argparse.ArgumentParser(
        description="Gathering values from several veritracer executions", add_help=True)
    parser.add_argument('-f','--filename', type=str, default="veritracer.dat")
    parser.add_argument('-o','--output', type=str, default="veritracer.csv")
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

    filesize = get_filesize(args)
    # Dict which maps bt_name to associated directories
    dict_bt_files = partition_samples(args)
    
    for bt_name, dir_list in dict_bt_files.iteritems():

        print bt_name, dir_list
        
        global offset, local_offset
        offset = 0
        local_offset = 0
        
        # list_files = get_files(args)
        list_files = map(lambda dir_ : dir_ + os.sep + args.filename, dir_list)
        filesize =  os.path.getsize(list_files[0])
        # statsFunctions = crange_tracer_stat.load_functions(len(list_files))
        output_file = args.output + "." + bt_name

        n_ = filesize / size_to_read + 1
        for i in xrange(n_):
            # Parse dir
            list_exp = parse_directory(args, list_files)

            # Parse values
            # list_errors = parse_values(statsFunctions, list_exp)
            list_errors = parse_values(list_exp)

            # Write csv
            output_file_i = output_file + "_" + str(i)
            write_csv(output_file_i, list_errors)
                                
if '__main__' == __name__:
    
    main()
