#!/usr/bin/env python

import sys
import argparse
import os
import multiprocessing as mp
import subprocess
import errno

def init_module(subparsers, veritracer_plugins):
    veritracer_plugins["launch"] = run
    analyzer_parser = subparsers.add_parser("launch", help="Launch parallel executions of binary")
    analyzer_parser.add_argument('-b','--binary', type=str, default="veritracer.dat", metavar='',
                                 help="filename of the trace to gather")
    analyzer_parser.add_argument('-j','--jobs', type=int, required=True, metavar='',
                                 help='number of parallel jobs to run')
    analyzer_parser.add_argument('--prefix-dir', type=str, default="", metavar='',
                                 help='prefix of the directory')
    analyzer_parser.add_argument('-f','--force',  action="store_true", 
                                 help='ignore existing directories')
    analyzer_parser.add_argument('--verbose', action="store_true",
                                 help="verbose mode")


def make_directory(dirname, force=False):
    dirname_str = str(dirname)
    try:
        os.mkdir(dirname_str)
    except OSError as err:
        if force and err.errno == errno.EEXIST and os.path.isdir(dirname_str):
            pass
        else:
            print err
            exit(1)

def cd(dirname):
    dirname_str = str(dirname)
    try:
        os.chdir(dirname_str)
    except OSError as err:
        print err
        exit(1)
        
def run_binary(args):
    num_dir, binary, force = args
    make_directory(num_dir, force)
    cd(num_dir)
    binary = "../{binary}".format(binary=binary)
    try:
        subprocess.check_output(binary, stderr=subprocess.STDOUT, shell=True)
    except subprocess.CalledProcessError as err:
        print err
        exit(1)
        
def launch(binary_path, args):
    jobs = args.jobs
    force = True if args.force else False

    jobs_pool = mp.Pool(processes=jobs)
    binary_args = [(i,binary_path, force) for i in range(1,jobs+1)]
    re = jobs_pool.map(run_binary, binary_args)

    jobs_pool.close()
    jobs_pool.join()
        
def run(args):

    binary_path = args.binary
    force = True if args.force else False
    
    if args.prefix_dir:
        prefix_dir = args.prefix_dir
        make_directory(prefix_dir, force)
        cd(prefix_dir)
        binary_path = "../{binary}".format(binary=binary_path)
        
    launch(binary_path, args)
        
    return True
