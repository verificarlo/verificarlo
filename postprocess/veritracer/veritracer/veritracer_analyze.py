#!/usr/bin/env python

import sys
import argparse
import os
import csv
from collections import namedtuple

import veritracer_backtrace as vtr_backtrace
import veritracer_math as vtr_math
import veritracer_format.binary as fmtbinary
import veritracer_format.text as fmttext
import veritracer_format.veritracer_format as vtr_fmt
import veritracer_branchement as vtr_br

csv_header = ["hash","type","time","max","min","median","mean","std","significant_digit_number"]

local_offset = 0

StatsLine = namedtuple('StatsLine',['hash','type','time',
                                    'max','min','mean',
                                    'median','std','significant_digit_number'])

default_traces_path=".vtrace"

def init_module(subparsers, veritracer_plugins):
    veritracer_plugins["analyze"] = run
    analyze_parser = subparsers.add_parser("analyze", help="Gathering values from several veritracer executions")
    analyze_parser.add_argument('--filename', type=str, default="veritracer.dat", metavar='',
                                 help="filename of the trace to gather")
    analyze_parser.add_argument('-o','--output', type=str, default="veritracer", metavar='',
                                 help='output filename')
    # analyze_parser.add_argument('-N','--read-bytes', type=int, default=0, metavar='',
    #                              help='read the N first bytes')
    analyze_parser.add_argument('--prefix-dir', type=str, default=default_traces_path, metavar='',
                                 help='prefix of the directory to analyze (default {default})'.format(
                                     default=default_traces_path))
    analyze_parser.add_argument('--backtrace-filename', type=str, default="backtrace.dat", metavar='',
                                 help='filename of the backtrace to use')
    analyze_parser.add_argument('--verbose', action="store_true",
                                 help="verbose mode")
    analyze_parser.add_argument('--format', action='store', choices=['binary','text'], default='binary',
                                 help='veritracer.dat encoding format')
    analyze_parser.add_argument('--filename-branch', type=str, default="branchement.dat", metavar='',
                                help='filename of the branches trace to gather')
    analyze_parser.add_argument('--output-branch', type=str, default="branchement", metavar='',
                                help="branchement output filename")
    
    
def parse_file(args, filename):
    if args.format == "binary":
        return fmtbinary.parse_file(filename)
    else:
        return fmttext.parse_file(filename)

def parse_exp(exp):
    format = map(lambda valueLine : valueLine.format, exp)
    time = map(lambda valueLine : valueLine.time, exp)
    address = map(lambda valueLine : valueLine.address, exp)
    hashv = map(lambda valueLine : valueLine.hash, exp)
    value = map(lambda valueLine : valueLine.value, exp)

    valueline = vtr_fmt.ValuesLine(format=format[0],
                                   time=time[0],
                                   address=address[0],
                                   hash=hashv[0],
                                   value=value)

    return valueline

def parse_directory(args, list_files):
    list_exp = map(lambda file : parse_file(args, file), list_files)
    list_exp = filter(lambda x : x != None, list_exp)   
    value_exp = [parse_exp(exp) for exp in zip(*list_exp)]
    return value_exp

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
    hashv        = values_list.hash
    FP_list      = values_list.value
    mean_   = vtr_math.mean(FP_list)
    std_    = vtr_math.std(mean_, FP_list)
    median_ = vtr_math.median(FP_list)
    max_    = max(FP_list)
    min_    = min(FP_list)    
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
    
def parse_values(exp_list):    
    return map(lambda x : compute_stats(x), exp_list)

def open_csv(filename):
    output = filename
    fo = open(output, 'wb')
    csv_writer = csv.DictWriter(fo, fieldnames=csv_header)
    csv_writer.writeheader()
    return csv_writer

def write_csv(filename, values_list):
    csv_writer = open_csv(filename)
    for row in values_list:
        csv_writer.writerow(row._asdict())

def run(args):

    if args.prefix_dir and os.path.isdir(args.prefix_dir):
        os.chdir(args.prefix_dir)
    else:
        print "Unknown directory {dir}".format(dir=args.prefix_dir)
        return False

    # Dict which maps bt_name to associated directories
    try:
        dict_bt_files = vtr_backtrace.partition_samples(args)
    except vtr_backtrace.BacktraceFileNoExist as bt_error:
        print "Error: backtrace file {bt} does not exist".format(bt=bt_error.message)
        return False
    except vtr_backtrace.AllEmptyFile as empty_error:
        print "Error: all {filename} files are empty".format(filename=empty_error.message)
        return False
    
    for bt_name, dir_list in dict_bt_files.iteritems():

        if args.verbose:
            print bt_name, dir_list
                
        fp_data_files_list = map(lambda d : d + os.sep + args.filename, dir_list)
        branch_files_list = map(lambda d : d + os.sep + args.filename_branch, dir_list)
        
        filesize =  os.path.getsize(fp_data_files_list[0])
        fp_data_output_file = args.output + "." + bt_name
        branch_output_file = args.output_branch + "." + bt_name
        
        # nb_slices = filesize / args.read_bytes
        # for slc in xrange(nb_slices):

        # Parse dir
        fp_data_exp_list = parse_directory(args, fp_data_files_list)
        branch_exp_list = parse_directory(args, branch_files_list)
        
        # Parse values
        fp_data_values_list = parse_values(fp_data_exp_list)
        branch_values_list = vtr_br.parse_values(branch_exp_list)
        
        # Write csv
        # output_file_slc = output_file + "." + str(slc)
        output_file_slc = fp_data_output_file
        write_csv(output_file_slc, fp_data_values_list)
        vtr_br.write_csv(branch_output_file, branch_values_list)
        
            
    return True
