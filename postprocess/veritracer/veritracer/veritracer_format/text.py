#!/usr/bin/env python

import os
import veritracer_format as vtr_fmt
import veritracer.veritracer_math as vtr_math

# Return size in bytes associated to the given format
def parse_format(fmt):
    if fmt.find('32') != -1:
        return 4
    elif fmt.find('64') != -1:
        return 8
    else:
        print "Error: unknown format %s" % fmt
        exit(1)

# Args order fmt,time,hash,ptr,val
def parse_raw_line(line):

    line_split = line.split()

    size  = parse_format(line_split[0])
    ptr   = line_split[3]
    time  = int(line_split[1])
    hashv = int(line_split[2])        
    val = vtr_math.hexa_to_fp(line_split[4])

    value = vtr_fmt.ValuesLine(format=size,
                               time=time,
                               address=ptr,
                               hash=hashv,
                               value=val) 
    return value

def parse_file(filename):
    try:
        fi = open(filename, 'r+b')
    except IOError as e:
        print "Could not open " + filename + " " + str(e)
        return

    if os.path.getsize(filename) == 0:
        print "Error: %s is empty" % filename
        exit(1)

    values_list = [parse_raw_line(line) for line in fi]
    return values_list
