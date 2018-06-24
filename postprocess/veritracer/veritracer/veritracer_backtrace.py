#!/usr/bin/python

import os
import re
from collections import namedtuple

class BacktraceFileNoExist(Exception):
    def __init__(self,message):
        super(BacktraceFileNoExist,self).__init__(message)

class AllEmptyFile(Exception):
    def __init__(self,message):
        super(AllEmptyFile,self).__init__(message)
        
BacktraceLine = namedtuple('BacktraceLine', ['binary_path','function','address'])
empty_backtrace_line = BacktraceLine('','','')
backtrace_separator = "###"
backtrace_dirname = ".backtrace"
backtrace_map_filename = "backtraces.map"
backtrace_reduced_map_filename = "backtraces_reduced.map"
backtrace_map_file = None
ignored_dir_list = ['.vtrace',backtrace_dirname]

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
        # address_ = re.findall('(?<=\[).*(?=\])',line)
        # address = address_[0] if len(address_) != 0 else ""
        address = ""
    except Exception as e:
        print "Error while parsing backtrace line: {line}".format(line=line)
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
            else: # hash_value + backtrace_separator 
                str_to_write = bt_line 
            bt_file.write(str_to_write)


# Return an hash value associated to a backtrace
def get_bt_hash(backtrace):
    bt_str = ""
    for bt in backtrace:
        bt_str += "".join(bt)
    return hash(bt_str)
        
# Dump a reduced version of the backtrace
# with a line per function
# Each backtrace ended with its hash value and a separator
def dump_reduced_backtrace(bt_filename, backtrace_list, visited_bt_hash, bt_file_map):

    bt_file = open(bt_filename+".rdc","w")
    
    backtrace_list_reduced = []
    bactrace_tmp = empty_backtrace_line
    backtrace_list_tmp = []
    count = 0
    
    for backtrace in backtrace_list:
        for bt_line in backtrace:
            if type(bt_line) == BacktraceLine:
                backtrace_list_tmp.append(bt_line)
            else: # hash_value + backtrace_separator
                hash_value = bt_line.split(backtrace_separator)[0]
                hash_bt = get_bt_hash(backtrace_list_tmp)
                backtrace_list_reduced.append((hash_value,hash_bt,count))

                if not hash_bt in visited_bt_hash:
                    visited_bt_hash.add(hash_bt)
                    for line in backtrace_list_tmp:
                        bt_file_map.append("{binary}({function})\n".format(
                            binary=line.binary_path,
                            function=line.function))
                    bt_file_map.append("{hash}{sep}\n".format(
                        hash=hash_bt,
                        sep=backtrace_separator))
                    
                count += 1
                backtrace_list_tmp = []

    for t in backtrace_list_reduced:
        bt_file.write( "%s %s %d\n" % t )

    return (visited_bt_hash,bt_file_map)

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

def check_line(line):
    return len(set(line)) == 1

# Partion the list according to theire bt size
def split_bt_list(bt_list, counter):
    # get the different lines
    diff_lines = set(map(lambda bt:bt[counter], bt_list))

    bt_list_splitted = []
    for line in diff_lines:
        bt_list_filtered = filter(lambda bt: bt[counter] == line, bt_list)
        bt_list_splitted.append(map(lambda bt : bt[counter+1:], bt_list_filtered))

    return filter(lambda bt : bt != [],bt_list_splitted)

# WIP
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

# print sequence of file with ellipse (...)
# [1,2,3,4] -> 1...4
def pretty_printer_list(dirs_list):

    def fun_aux(seen, last=False):
        sep = '' if last else ','
        if len(seen) == 1:
            return "{v}{sep}".format(v=seen[0],sep=sep)
        elif len(seen) == 2:
            return "{v1},{v2}{sep}".format(v1=seen[0],v2=seen[1],sep=sep)
        else:
            return "{v1}...{v2}{sep}".format(v1=seen[0],v2=seen[-1],sep=sep)
    
    try:
        dirs_list_int = map(int, dirs_list)
        sorted_dirs_list_int = sorted(dirs_list_int, reverse=True)
        seen = [sorted_dirs_list_int.pop()]
        s = "["
        while  sorted_dirs_list_int != []:
            d = sorted_dirs_list_int.pop()
            if d == seen[-1] + 1:
                seen.append(d)
            else:
                s += fun_aux(seen)
                seen = [d]
        s += fun_aux(seen, last=True) + "]"
        return s
    except ValueError as err:
        return dirs_list
        
def get_dir_list(args, getBacktraceDir=False):
    
    dir_list = sorted(os.listdir("."), key=lambda d : int(d) if d.isdigit() else d)
    filtered_dir_list = filter(lambda d : not d in ignored_dir_list, dir_list)
    dirs = filter(os.path.isdir, filtered_dir_list)

    isfile = lambda filename : lambda d : os.path.isfile(d+os.sep+filename) 
    isnotfile = lambda filename : lambda d : not os.path.isfile(d+os.sep+filename) 
    isemptyfile = lambda filename : lambda d : os.path.getsize(d+os.sep+filename) == 0
    isnotemptyfile = lambda filename : lambda d : os.path.getsize(d+os.sep+filename) != 0
    
    dirs_with_veritracer_file = filter(isfile(args.filename), dirs)
    dirs_without_veritracer_file = filter(isnotfile(args.filename), dirs)
    dirs_with_empty_veritracer_file = filter(isemptyfile(args.filename),
                                             dirs_with_veritracer_file)
    dirs_with_notempty_veritracer_file = filter(isnotemptyfile(args.filename),
                                                dirs_with_veritracer_file)

    dirs_with_backtrace_file = filter(isfile(args.backtrace_filename), dirs)
    dirs_without_backtrace_file = filter(isnotfile(args.backtrace_filename), dirs)
    dirs_with_empty_backtrace_file = filter(isemptyfile(args.backtrace_filename),
                                            dirs_with_backtrace_file)


    if dirs_with_notempty_veritracer_file == []:
        raise(AllEmptyFile(args.filename))
    
    if  dirs_without_veritracer_file != []:
        print "Warning: directories do not have {veritracer_file} files {empty_list}".format(
            veritracer_file=args.filename,
            empty_list=pretty_printer_list(dirs_without_veritracer_file))

    if  dirs_with_empty_veritracer_file != []:
        print "Warning: {veritracer_file} files are empty in these directories {empty_list}".format(
            veritracer_file=args.filename,
            empty_list=pretty_printer_list(dirs_with_empty_veritracer_file))
            
    if dirs_without_backtrace_file != []:
        print "Warning: directories do not have {backtrace_file} files {empty_list}".format(
            backtrace_file=args.backtrace_filename,
            empty_list=pretty_printer_list(dirs_without_backtrace_file))

    if  dirs_with_empty_backtrace_file != []:
        print "Warning: {backtrace_file} files are empty in these directories {empty_list}".format(
            backtrace_file=args.backtrace_filename,
            empty_list=pretty_printer_list(dirs_with_empty_backtrace_file))

    return dirs_with_backtrace_file if getBacktraceDir else dirs_with_veritracer_file

def dump_bt_reduced_map(filename, bt_file_map):
    fo = open(filename, "w")
    for line in bt_file_map:
        fo.write(line)
        
# We partition the samples according to their backtrace list
def partition_samples(args):
    
    # Directories list containing a veritracer.dat file
    # (or a backtrace.dat file if getBacktraceDir=True)
    dir_list = get_dir_list(args)
    
    # Dict which maps a sample/directory to its backtrace
    dir_bt_dict = {}
    for dir_ in dir_list:
        backtrace_list_filename = dir_ + os.sep + args.backtrace_filename
        if not os.path.isfile(backtrace_list_filename):
            raise(BacktraceFileNoExist(backtrace_list_filename))
        backtrace_list = parse_backtrace(backtrace_list_filename)
        dir_bt_dict[dir_] = backtrace_list
        
    # Create directory for gathering backtrace information
    if not os.path.exists(backtrace_dirname):
        os.makedirs(backtrace_dirname)

    # we order samples by their backtrace list size
    # it is useless to compare backtraces which have not the same size
    dict_vi = dir_bt_dict.viewitems()
    i = 0

    dict_to_return = {}

    visited_hash_bt = set()
    bt_file_map = []
    
    while dir_bt_dict != {}:
        (dir_,bt_list) = dir_bt_dict.popitem()
        dirs_list = [dir_]
        dirs_with_same_bt_size_list = filter(lambda (_,btl) : len(btl) == len(bt_list), dict_vi)
        for dirs_,bt_list_to_cmp in dirs_with_same_bt_size_list:
            if is_same_backtrace_list(bt_list, bt_list_to_cmp):
                dirs_list.append(dirs_)
                dir_bt_dict.pop(dirs_)

        backtrace_list_name = "%03dbt" % i
        add_backtrace_mapping(backtrace_list_name, dirs_list)
        backtrace_list_filename = backtrace_dirname + os.sep + backtrace_list_name
        dump_backtrace_list(backtrace_list_filename, bt_list)
        (visited_hash_bt,bt_file_map) = dump_reduced_backtrace(backtrace_list_filename,
                                                               bt_list,
                                                               visited_hash_bt,
                                                               bt_file_map)        
        dict_to_return[backtrace_list_name] = dirs_list
        i += 1


    backtrace_list_reduced_filename = backtrace_dirname + os.sep + backtrace_reduced_map_filename
    dump_bt_reduced_map(backtrace_list_reduced_filename, bt_file_map)
    
    return dict_to_return
