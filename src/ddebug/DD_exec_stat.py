import sys
import os
import time

class exec_stat:
    def __init__(self,repName):
        self.repName=repName
        self.timeInit()

    def terminate(self):
        self.timeEnd()
        self.printElapsed(int(self.end- self.start))
        self.printNbRun()

    def timeInit(self):
        self.start = time.time()

    def timeEnd(self):
        self.end = int(time.time())

    def printElapsed(self,duration):
        s= duration % 60
        rm= duration //60
        m=rm%60
        rh=rm//60
        h=rh%24
        rd=rh//24
        print ("\nElapsed Time: %id %ih %imin %is   "%(rd,h,m,s) )

    def isNew(self, filename):
        return ((os.stat(filename).st_mtime) > self.start)

    def printNbRun(self,dirName="."):
        import glob

        runTab=glob.glob(dirName+"/"+self.repName+"/*/dd.run*/dd.run.out")
        runFilter=[filename for filename in runTab if self.isNew(filename)]
        print(self.repName+"  search : %i run (with cache included: %i)"%(len(runFilter),len(runTab)) )
