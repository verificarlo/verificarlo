#!/usr/bin/python3
#coding: UTF-8
import babeltrace
from babeltrace import CTFWriter
import sys
# from tqdm import tqdm
import random
import string
import os

#Used to generate ID for the variable having no context info
def id_generator(size=6, chars=string.ascii_uppercase + string.digits):
    return ''.join(random.choice(chars) for _ in range(size))

#Remove the whitespace at the beginning and the end of a string
def removeWhitespace(string):
    return string.rstrip().lstrip()

#Linked list used to extract the callpath from backtrace file
class Callpath:
    def __init__(self, line, callpathNext):
        self.line = line
        self.callpathNext = callpathNext

class Context:
    def __init__(self, type_f, function, line, name):
        self.type_f = type_f
        self.function = function
        self.line = line
        self.name = name
        self.file = ""
        self.isWritten = False

    def eventWritten(self):
        if(self.isWritten == False):
            self.isWritten = True
            return False
        return True

#Check if the user entered a name for the output directory
if len(sys.argv) == 2:
    DirOut = sys.argv[1]
else:
    exit("Error : You need to give a name for the output directory.")

#Name of the file containing needed information to create the CTF trace
#The user need to use the python program at the root containing the .vtrace directory from VeriTracer
pathBacktrace = ".vtrace/.backtrace/backtraces_reduced.map"
pathLink = ".vtrace/.backtrace/000bt.rdc"
pathContext = "locationInfo.map"
pathData = ".vtrace/veritracer.000bt"

#If the backtrace exist
if os.path.exists(pathBacktrace) and (os.path.getsize(pathBacktrace) > 0):
    backtraceExist = True
else:
    backtraceExist = False
print("Backtrace ", backtraceExist)

#If locationInfo.map exist
if os.path.exists(pathContext):
    contextExist = True
else:
    contextExist = False
print("Context ", contextExist)

#output.csv.000bt contains data of event on a floating point variable
fileEvents = open(pathData, "r")
#locationInfo.map contains the context of a variable indexed with the hash of the variable
if(contextExist):
    fileContext = open(pathContext, "r")


if(backtraceExist):
    #Here, we build the link between the hash of the variable and the hash of the backtrace
    #The hash of the variable is the key to the hash of the corresponding backtrace
    #000bt.rdc contains the link between the hash of the variable and the hash of it's corresponding backtrace
    fileLink = open(pathLink, "r")
    link = dict()
    for line in fileLink.readlines():
        tmp = line.split("\n")[0].split(" ")
        link[int(tmp[0])] = tmp[1]
        
    #backtraces_reduced.map contains the backtraces of the programs and their hash
    fileBacktrace  = open(pathBacktrace, "r")    
    backtrace = dict()
    act = None
    #The backtrace here is a None-terminated linked list with the root being stored in the dictionnary backtrace and indexed by the backtrace hash
    for line in fileBacktrace.readlines():
        if "#" in line: #If we encounter a # we have the hash and the backtrace is read
            hashbt = line.split("#")[0]
            backtrace[hashbt] = act
            act = None
        else: #else we continue reading until we have the complete backtrace
            oldAct = act
            act = Callpath(line.split("\n")[0], oldAct)
    fileLink.close()
    fileBacktrace.close()

contexts = dict() #We store the context information in a Context Object in a dict indexed with the hash of the variable to easily link with the context
if(contextExist): #If we have locationInfo.map we can extract context information for the variables
    for line in fileContext.readlines():
        tmp = line.split("\n")[0].split(":") #in the line we have "hash: <things>" so we split the hash and the other information here
        info = tmp[1].split(";") #We split the information which are separated with a ";" character
        if "binary32" in info[0]: #we identify the type which is in string format to transform in byte
            type_f = "4"
        elif "binary64" in info[0]:
            type_f = "8"
        else:
            type_f = "0"
    
        fileAndLine = info[2].split(" ") #we split the source file name and the line.column
        files = None
        if len(fileAndLine) == 3: #If there is only the line information (legacy from when we couldn't have the name of the source for every variable)
            line = int(fileAndLine[1])
        elif len(fileAndLine) == 4: 
            files = fileAndLine[1]
            line = int(fileAndLine[2].split(".")[0]) #Extract filename and line in that file
        contexts[int(tmp[0])] = Context(type_f, removeWhitespace(info[1]), line, removeWhitespace(info[3])) #And we create and store the context object in the dict
        if(not files is None):
            contexts[int(tmp[0])].file = files
        # contexts[int(tmp[0])] = Context(type_f, info[1], 0, info[3])
    fileContext.close()

#To remove the header of the file containing the events
print(fileEvents.readline().split("\n")[0])

writer = CTFWriter.Writer(DirOut) #The writer for the CTF output
writer.add_environment_field("domain", "veritrace") #We had this information in the metadata environment to tell we are a VeriTracer trace

#We define a clock for the trace and assign it to the writer instance
clock = CTFWriter.Clock('my_clock')
clock.description = 'my clock'
clock.freq = 1
writer.add_clock(clock)

#We define a stream type
stream_class = CTFWriter.StreamClass('my_stream')
stream_class.clock = clock

#We create the events composing a CTF VeriTracer trace
value_event = CTFWriter.EventClass('value')
callpath_event = CTFWriter.EventClass('callpath')
context_event = CTFWriter.EventClass('context')

#Declaration of types common for the different events
string_f = CTFWriter.StringFieldDeclaration()
id_field = CTFWriter.IntegerFieldDeclaration(64)
id_field.signed = False

#Types for value events
double_f = CTFWriter.FloatFieldDeclaration()
double_f.exponent_digits = CTFWriter.FloatFieldDeclaration.DBL_EXP_DIG
double_f.mantissa_digits = CTFWriter.FloatFieldDeclaration.DBL_MANT_DIG
int_f = CTFWriter.IntegerFieldDeclaration(32)
int_f.signed = True


#add the fields to the callpath events
callpath_event.add_field(id_field, "parent")
callpath_event.add_field(id_field, "id")
callpath_event.add_field(string_f, 'name')

#The fields for the value events
value_event.add_field(id_field, "context")
value_event.add_field(id_field, "parent")
value_event.add_field(double_f, "mean")
value_event.add_field(double_f, "max")
value_event.add_field(double_f, "min")
value_event.add_field(double_f, "std")
value_event.add_field(double_f, "median")
value_event.add_field(double_f, "significant_digits")

#The fields for the context event
context_event.add_field(id_field, "id")
context_event.add_field(string_f, "file")
context_event.add_field(string_f, "name")
context_event.add_field(string_f, "function")
context_event.add_field(int_f, "line")
context_event.add_field(string_f, "type")

#We add the event to the stream
stream_class.add_event_class(callpath_event)
stream_class.add_event_class(context_event)
stream_class.add_event_class(value_event)

#we use a unique stream, a stream is one binary file containing data of a trace
stream = writer.create_stream(stream_class)

#We write a value event in stream
def createvalueEvent(stream, contexts, hash_f, parent, type_f, max_f, min_f, median_f, mean_f, std_f, significant_digits):
    event = CTFWriter.Event(value_event)

    context = getContextEvent(stream, contexts, type_f, hash_f)
    
    if not context is None:
        event.payload("context").value = context
    else:
        event.payload("context").value = 0
    event.payload("parent").value = parent
    event.payload("max").value = max_f
    event.payload("min").value = min_f
    event.payload("median").value = median_f
    event.payload("mean").value = mean_f
    event.payload("std").value = std_f
    event.payload("significant_digits").value = significant_digits
    stream.append_event(event)


#return the context identifier (the hash of the variable from VeriTracer) and create the context object if it doesn't exist, and write it in the trace if it hasn't already been done
def getContextEvent(stream, contexts, type_f, hash_f):
    if(hash_f in contexts): #If the context for this variable exist
        context = contexts[hash_f]
        #We extract it and look if it has already been written inside the CTF trace
        if not context.eventWritten():
            event = CTFWriter.Event(context_event)
            event.payload("id").value = hash_f
            event.payload("file").value = context.file
            event.payload("name").value = context.name
            event.payload("function").value = context.function
            event.payload("line").value = context.line
            event.payload("type").value = context.type_f
            stream.append_event(event)
        return hash_f
    else:
        contexts[hash_f] = Context(type_f, "", 0, id_generator())
        return getContextEvent(stream, contexts, type_f, hash_f) #We are recalling the function to insure it has been written in the CTF trace

#Clean the name of a function in the callpath
def extractName(act):
    name = act.line.split("/")
    name = name[len(name)-1]
    name = name.split("(")
    name = name[len(name)-1]
    name = name.split("+")[0] 
    # name = name.split(")")[0]
    return name

#merge the various backtrace to obtain a backtrace which can be written in the CTF trace
def createCallpathEvent(stream, backtraces):
    id_callpath_avail = 1 #First available id, 0 is reserved so that the root can have it as parent
    fabricatedBacktrace = dict()
    for key in backtraces:
        act = backtraces[key] #We begin with the root of the backtrace linked list
        oldAct = None #Used to know the parent
        while not act is None:
            name = extractName(act)
            if not name in fabricatedBacktrace:
                fabricatedBacktrace[name] = id_callpath_avail
                id_callpath_avail += 1
                eventCallpath = CTFWriter.Event(callpath_event)
                
                eventCallpath.payload('name').value = name
                if not oldAct is None: #If it's the root
                    eventCallpath.payload('parent').value = fabricatedBacktrace[extractName(oldAct)]
                else:
                    eventCallpath.payload("parent").value = 0
                eventCallpath.payload('id').value = fabricatedBacktrace[name]
                stream.append_event(eventCallpath)
            oldAct = act
            act = act.callpathNext
    # for act in fabricatedBacktrace:
    #     print(act, " ", fabricatedBacktrace[act])
    return fabricatedBacktrace

#return the identifier of this callpath inside the CTF trace
def linkBacktrace(hash_f, link, callpaths, fabricatedBacktrace):
    if(hash_f in link):

        hash_bt = link[hash_f] #Give us the backtrace's hash for this particular variable
        oldAct = None #Used to eliminate get_backtrace call
        act = callpaths[hash_bt] #Give us the root of this backtrace
        while not act is None:
            oldoldAct = oldAct
            oldAct = act
            act = act.callpathNext
    #We get to the end of the backtrace to know the direct parent of the variable and use the sorted backtrace to obtain the unique identifier
        return fabricatedBacktrace[extractName(oldoldAct)]
    return 0

buf = 0
timecallpath = 0
fabricatedBacktrace = None
line = fileEvents.readline()

# for line in tqdm(fileEvents.readlines()):
while line != "":
    buf += 1

    info = line.split(",") #We parse the CVS event
    
    clock.time = int(info[2]) * 1000
    
    #The first time, we create the full backtrace inside the CTF trace
    if(timecallpath == 0):
        timecallpath = 1
        if(backtraceExist):
            fabricatedBacktrace = createCallpathEvent(stream, backtrace)   

    hash_f = int(info[0]) #The hash of the variable this event is linked to
    if(backtraceExist): #If we have the backtrace we get the identifier for this variable
        parent = linkBacktrace(hash_f, link, backtrace, fabricatedBacktrace) #We identify the correct parent in the fabricatedBacktrace to put in the context
    else:
        parent = 0
    #We create the event in the trace
    #if the context of this variable isn't already written, it will be written in the trace before the value event
    createvalueEvent(stream, contexts, hash_f, parent, info[1], float(info[3]), float(info[4]), float(info[5]), float(info[6]), float(info[7]), float(info[8]))
    
    line = fileEvents.readline()
    if(buf == 400): #We write in the stream file every 400 events to ensure a low amount of used ram and a file with the minimal amount of padding possible.
         #stream.flush() write the packet streams on the disk and add padding to ensure the size of the stream. if we flush too much the files will be very big
         #400 is a number deduced to maximize the number of event in one packet and minimize the padding
         stream.flush()
         buf = 0

stream.flush()

fileEvents.close()
