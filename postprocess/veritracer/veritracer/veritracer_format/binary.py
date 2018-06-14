#!/usr/bin/env python

import os
import mmap
import struct
from collections import deque

import veritracer_format as vtr_fmt

# Args order fmt,time,hash,ptr,val
def parse_raw_line(line):
    # print line
    rawvalue = vtr_fmt.ValuesLine(format=line[0],
                                  time=line[1],
                                  hash=line[3],
                                  address=line[2],
                                  value=line[4])
    
    size  = rawvalue.format
    ptr   = "0x0" if rawvalue.address == "(nil)" else rawvalue.address
    time  = int(rawvalue.time)
    hashv = int(rawvalue.hash)        
    val   = rawvalue.value
    value = vtr_fmt.ValuesLine(format=size,
                               time=time,
                               address=ptr,
                               hash=hashv,
                               value=val)
    
    return value
        
def parse_file(filename):

    try:
        f = open(filename, 'r+b')
    except IOError as e:
        print "Could not open " + filename + " " + str(e)
        return

    if os.path.getsize(filename) == 0:
        print "Error: %s is empty" % filename
        exit(1)

    # global offset
    mm = mmap.mmap(f.fileno(), 0)    
    # mm.seek(vtr_fmt.offset)
    
    values_list = deque()
    append = values_list.append
    
    size_file = mm.size()

    try:
        # while vtr_fmt.size_to_read + vtr_fmt.offset >  mm.tell() :
        while size_file > mm.tell():
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
            values_line = parse_raw_line(sizeVl+values)
            append(values_line)

    except Exception as e:
        # if vtr_fmt.size_to_read + vtr_fmt.offset <  mm.tell():
        #     print 'Parse_file ERRROR ' + str(e)
        # else:
        print e
        #     exit(1)
            
    # global local_offset
    # local_offset = mm.tell()
    mm.close()
    return values_list
