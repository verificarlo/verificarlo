#!/usr/bin/env python3
#coding: UTF-8
# import babeltrace
from babeltrace import CTFWriter
import tempfile
import sys
from tqdm import tqdm
import random
import string
import os

def id_generator(size=6, chars=string.ascii_uppercase + string.digits):
    return ''.join(random.choice(chars) for _ in range(size))

if len(sys.argv) == 3:
    fileInName = sys.argv[1]
    trace_path = sys.argv[2]
else:
    exit("Erreur : Vous devez donner un fichier d'entrée et un emplacement pour créer la trace CTF.")

# fileIn = open(fileInName, "r")

def reverse_file(in_filename, fout, blocksize=1024):
    filesize = os.path.getsize(in_filename)
    fin = open(in_filename, 'rb')
    for i in range(filesize // blocksize, -1, -1):
        fin.seek(i * blocksize)
        data = fin.read(blocksize)
        fout.write(data[::-1])

def enumerate_reverse_lines(in_filename, blocksize=1024):
    fout = tempfile.TemporaryFile()
    reverse_file(in_filename, fout, blocksize=blocksize)
    fout.seek(0)
    for line in fout:
        yield line[::-1]

for line in enumerate_reverse_lines(fileInName):
    print(line)

# fileIn.close()
# fileTmp.close()
#
# fileIn = open("tmp.0001", "r")
#
# writer = CTFWriter.Writer(trace_path)
# writer.add_environment_field("domain", "kernel")
#
# clock = CTFWriter.Clock('my_clock')
# clock.description = 'my clock'
# clock.freq = 1
# writer.add_clock(clock)
#
# stream_class = CTFWriter.StreamClass('my_stream')
# stream_class.clock = clock
#
# context_event = CTFWriter.EventClass('sched_process_fork')
#
# #Variable d'événements commun
# name = CTFWriter.StringFieldDeclaration()
# id_field = CTFWriter.IntegerFieldDeclaration(64)
# id_field.signed = True
#
# context_event.add_field(id_field, "parent_tid")
# context_event.add_field(id_field, "parent_pid")
# context_event.add_field(id_field, "child_pid")
# # context_event.add_field(id_field, "prev_prio")
# # context_event.add_field(id_field, "prev_state")
# context_event.add_field(id_field, "child_tid")
# # context_event.add_field(id_field, "next_prio")
# context_event.add_field(name, 'parent_comm')
# context_event.add_field(name, 'child_comm')
#
# sequence_type = CTFWriter.SequenceFieldDeclaration(id_field, "_vtids_length_")
# context_event.add_field(id_field, "_vtids_length_")
# context_event.add_field(sequence_type, "vtids")
#
# # array_field = CTFWriter.ArrayFieldDeclaration(id_field, 5)
#
# stream_class.add_event_class(context_event)
#
# stream = writer.create_stream(stream_class)
#
# clock.time = 0
# info = "main"
# info2 = "init"
# event = CTFWriter.Event(context_event)
# event.payload("parent_comm").value = info
# event.payload("parent_tid").value = hash(info)
# event.payload("parent_pid").value = hash(info)
# event.payload("child_comm").value = info2
# event.payload("child_tid").value = hash(info2)
# event.payload("child_pid").value = hash(info2)
# event.payload("_vtids_length_").value = 3
# event.payload("vtids").length = event.payload("_vtids_length_")
# event.payload("vtids").field(0).value = hash("init")
# event.payload("vtids").field(1).value = hash("dot_well")
# event.payload("vtids").field(2).value = hash("dot_zero")
# stream.append_event(event)
#
# clock.time = 1
# info = "main"
# info2 = "dot_well"
# event = CTFWriter.Event(context_event)
# event.payload("parent_comm").value = info
# event.payload("parent_tid").value = hash(info)
# event.payload("parent_pid").value = hash(info)
# event.payload("child_comm").value = info2
# event.payload("child_tid").value = hash(info2)
# event.payload("child_pid").value = hash(info2)
# event.payload("_vtids_length_").value = 3
# event.payload("vtids").length = event.payload("_vtids_length_")
# event.payload("vtids").field(0).value = hash("init")
# event.payload("vtids").field(1).value = hash("dot_well")
# event.payload("vtids").field(2).value = hash("dot_zero")
# stream.append_event(event)
#
# clock.time = 2
# info = "main"
# info2 = "dot_zero"
# event = CTFWriter.Event(context_event)
# event.payload("parent_comm").value = info
# event.payload("parent_tid").value = hash(info)
# event.payload("parent_pid").value = hash(info)
# event.payload("child_comm").value = info2
# event.payload("child_tid").value = hash(info2)
# event.payload("child_pid").value = hash(info2)
# event.payload("_vtids_length_").value = 3
# event.payload("vtids").length = event.payload("_vtids_length_")
# event.payload("vtids").field(0).value = hash("init")
# event.payload("vtids").field(1).value = hash("dot_well")
# event.payload("vtids").field(2).value = hash("dot_zero")
# stream.append_event(event)
#
# clock.time = 3
# info = "dot_zero"
# info2 = "naive_dot_product"
# event = CTFWriter.Event(context_event)
# event.payload("parent_comm").value = info
# event.payload("parent_tid").value = hash(info)
# event.payload("parent_pid").value = hash(info)
# event.payload("child_comm").value = info2
# event.payload("child_tid").value = hash(info2)
# event.payload("child_pid").value = hash(info2)
# event.payload("_vtids_length_").value = 1
# event.payload("vtids").length = event.payload("_vtids_length_")
# event.payload("vtids").field(0).value = hash("naive_dot_product")
# stream.append_event(event)
# #
# # clock.time = 4
# # info = "dot_zero"
# # info2 = "dot_product_accurate"
# # event = CTFWriter.Event(context_event)
# # event.payload("parent_comm").value = info
# # event.payload("parent_tid").value = hash(info)
# # event.payload("parent_pid").value = hash(info)
# # event.payload("child_comm").value = info2
# # event.payload("child_tid").value = hash(info2)
# # event.payload("child_pid").value = hash(info2)
# # event.payload("_vtids_length_").value = 0
# # stream.append_event(event)
# #
# # clock.time = 5
# # info = "dot_well"
# # info2 = "dot_product_accurate"
# # event = CTFWriter.Event(context_event)
# # event.payload("parent_comm").value = info
# # event.payload("parent_tid").value = hash(info)
# # event.payload("parent_pid").value = hash(info)
# # event.payload("child_comm").value = info2
# # event.payload("child_tid").value = hash(info2)
# # event.payload("child_pid").value = hash(info2)
# # event.payload("_vtids_length_").value = 0
# # stream.append_event(event)
# #
# # clock.time = 6
# # info = "dot_well"
# # info2 = "naive_dot_product"
# # event = CTFWriter.Event(context_event)
# # event.payload("parent_comm").value = info
# # event.payload("parent_tid").value = hash(info)
# # event.payload("parent_pid").value = hash(info)
# # event.payload("child_comm").value = info2
# # event.payload("child_tid").value = hash(info2)
# # event.payload("child_pid").value = hash(info2)
# # event.payload("_vtids_length_").value = 0
# # stream.append_event(event)
#
# stream.flush()
# fileIn.close()
