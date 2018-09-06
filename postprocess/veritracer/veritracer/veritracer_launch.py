#!/usr/bin/env python

import sys
import argparse
import os
import multiprocessing as mp
import subprocess
import errno

default_traces_path=".vtrace"

def init_module(subparsers, veritracer_plugins):
    veritracer_plugins["launch"] = run
    launch_parser = subparsers.add_parser("launch", help="Launch parallel executions of binary")
    launch_parser.add_argument('-b','--binary', type=str, required=True, metavar='binary',
                                 help="filename of the trace to gather")
    launch_parser.add_argument('-j','--jobs', type=int, required=True, metavar='jobs',
                                 help='number of parallel jobs to run')
    launch_parser.add_argument('--prefix-dir', type=str, default=default_traces_path, metavar='',
                                 help='prefix of the directory')
    launch_parser.add_argument('-f','--force',  action="store_true", 
                                 help='ignore existing directories')
    # Without a default value, processes cannot be killed with keyboard interruption
    launch_parser.add_argument('--timeout', action="store", type=float, default=31536000)
    launch_parser.add_argument('--verbose', action="store_true",
                                 help="verbose mode")

def separate_dir(dirname):
    dirname_list = dirname.split(os.path.sep)
    dirname_list_cleaned = filter(lambda d : d != '.' and d != '', dirname_list)
    return dirname_list_cleaned

def pretty_printer_error(error):
    if error.errno == errno.EEXIST:
        print "Error: cannot create directory {dir}: File exists".format(dir=error.filename)
        print "       Use --force option to override it"
    else:
        print "Error: {err}: {filename}".format(err=error.strerror, filename=err.error.filename)

def is_error_to_ignore(error, dire, force):
    is_existing = error.errno == errno.EEXIST
    no_access = error.errno == errno.EACCES
    is_dir = os.path.isdir(dire)
    # We ignored error if 
    # is an existing file and force option is enable
    # or we don't have access permissions and it is an existing directory (ex: /home)
    return (force and is_existing) or (no_access and is_dir)

# force: override existing directories, if permissions are ok
# parents: make parent directories as needed
def make_directory(dirname, force=False, parents=False):
    dirname_str = str(dirname)

    if parents:
        dirs = separate_dir(dirname_str)
    else:
        dirs = [dirname_str]

    if os.path.isabs(dirname_str):
        cd(os.path.sep)
            
    for d in dirs:    
        try:
            os.mkdir(d)
            if parents:
                cd(d)
        except OSError as err:
            if is_error_to_ignore(err, d, force):
                if parents:
                    cd(d)
                else:
                    pass
            else:
                pretty_printer_error(err)
                return -1
        
def cd(dirname):
    dirname_str = str(dirname)
    try:
        os.chdir(dirname_str)
    except OSError as err:
        print err
        return -1
        
def run_binary(args):
    num_dir, binary, force = args
    make_directory(num_dir, force)
    cd(num_dir)
    binary = "{binary}".format(binary=binary)
    try:
        subprocess.check_output(binary, stderr=subprocess.STDOUT, shell=True)
        cd("..")
    except subprocess.CalledProcessError as err:
        print err
        return -1
        
def check_return_code(re, timeout):
    try:
        return_codes = re.get(timeout=timeout)
        for return_code in return_codes:
            if return_code == -1:
                exit(-1)
    except mp.TimeoutError:
        print "Error: timeout"
        exit(1)
        
def launch(binary_path, args):
    jobs = args.jobs
    force = True if args.force else False
    timeout = args.timeout

    try:
        jobs_pool = mp.Pool(processes=jobs)
        binary_args = [(i,binary_path, force) for i in range(1,jobs+1)]
        re = jobs_pool.map_async(run_binary, binary_args)
        check_return_code(re, timeout)
        jobs_pool.close()
    except KeyboardInterrupt:
        print "Warning: Interrupting processes"
        jobs_pool.terminate()
    jobs_pool.join()
    
def run(args):

    if args.jobs <= 0:
        print "Number of jobs must be positive"
        return False
    
    if not os.path.isabs(args.binary):
        pwd = os.getenv('PWD')
        binary_path = pwd + os.path.sep + args.binary
    else:
        binary_path = args.binary
        
    force = True if args.force else False
    
    if args.prefix_dir:
        prefix_dir = args.prefix_dir
        return_code = make_directory(prefix_dir, force, parents=True)
        if return_code == -1:
            exit(-1)

    launch(binary_path, args)
        
    return True
