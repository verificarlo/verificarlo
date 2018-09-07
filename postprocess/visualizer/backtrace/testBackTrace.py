#!/usr/bin/python
#coding: UTF-8
# import babeltrace
from babeltrace import CTFWriter
# import tempfile
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

fileIn = open(fileInName, "r")
# fileTmp = open("tmp.0001", "w")

def insert(originalfile,string):
    with open(originalfile,'r') as f:
        with open('newfile.txt','w') as f2:
            f2.write(string)
            f2.write(f.read())
    os.rename('newfile.txt',originalfile)

for line in fileIn.readlines():
    insert("tmp.0001", line)

fileIn.close()
# fileTmp.close()

fileIn = open("tmp.0001", "r")

writer = CTFWriter.Writer(trace_path)
writer.add_environment_field("domain", "kernel")

clock = CTFWriter.Clock('my_clock')
clock.description = 'my clock'
clock.freq = 1
writer.add_clock(clock)

stream_class = CTFWriter.StreamClass('my_stream')
stream_class.clock = clock

context_event = CTFWriter.EventClass('sched_process_fork')

#Variable d'événements commun
name = CTFWriter.StringFieldDeclaration()
id_field = CTFWriter.IntegerFieldDeclaration(64)
id_field.signed = True

context_event.add_field(id_field, "parent_tid")
context_event.add_field(id_field, "parent_pid")
context_event.add_field(id_field, "child_pid")
# context_event.add_field(id_field, "prev_prio")
# context_event.add_field(id_field, "prev_state")
context_event.add_field(id_field, "child_tid")
# context_event.add_field(id_field, "next_prio")
context_event.add_field(name, 'parent_comm')
context_event.add_field(name, 'child_comm')


stream_class.add_event_class(context_event)

stream = writer.create_stream(stream_class)

clock.time = 0

# event = CTFWriter.Event(context_event)
# event.payload("parent_comm").value = "ibus_daemon"
# event.payload("prev_tid").value = 1256
# event.payload("prev_prio").value = 20
# event.payload("next_prio").value = 20
# event.payload("prev_state").value = 1
# event.payload("child_comm").value = "gdbus"
# event.payload("child_tid").value = 1258
#
# stream.append_event(event)

# stream.flush()
time = 0
buf = 0
line = fileIn.readline()
line2 = fileIn.readline()
while(line2 != ""):
    buf += 1
    if("#" in line2):
        line2 = fileIn.readline()
        line = line2
        line2 = fileIn.readline()
    else:
        event = CTFWriter.Event(context_event)
    # print(line)
    # print(line2)
        tmp = line.split("../")
        info = tmp[len(tmp)-1].split("\n")[0]
        tmp2 = line2.split("../")
        info2 = tmp2[len(tmp2)-1].split("\n")[0]
        clock.time = time
        time += 1
        event.payload("parent_comm").value = info
        event.payload("parent_tid").value = hash(info)
        event.payload("parent_pid").value = hash(info)
        event.payload("child_comm").value = info2
        event.payload("child_tid").value = hash(info2)
        event.payload("child_pid").value = hash(info2)

        stream.append_event(event)
        if(buf == 1000000):
            stream.flush()
            buf = 0
        line = line2
        line2 = fileIn.readline()

stream.flush()
fileIn.close()
