import sys
import os

import subprocess

import shutil
import hashlib
import copy
from . import DD


def runCmdAsync(cmd, fname, envvars=None):
    """Run CMD, adding ENVVARS to the current environment, and redirecting standard
    and error outputs to FNAME.out and FNAME.err respectively.

    Returns CMD's exit code."""
    if envvars is None:
        envvars = {}

    with open("%s.out"%fname, "w") as fout:
        with open("%s.err"%fname, "w") as ferr:
            env = copy.deepcopy(os.environ)
            for var in envvars:
                env[var] = envvars[var]
            return subprocess.Popen(cmd, env=env, stdout=fout, stderr=ferr)

def getResult(subProcess):
    subProcess.wait()
    return subProcess.returncode


def runCmd(cmd, fname, envvars=None):
    """Run CMD, adding ENVVARS to the current environment, and redirecting standard
    and error outputs to FNAME.out and FNAME.err respectively.

    Returns CMD's exit code."""

    return getResult(runCmdAsync(cmd,fname,envvars))



class InterflopTask:

    def __init__(self, dirname, refDir,runCmd, cmpCmd,nbRun, maxNbPROC, runEnv):
        self.dirname=dirname
        self.refDir=refDir
        self.runCmd=runCmd
        self.cmpCmd=cmpCmd
        self.nbRun=nbRun
        self.FAIL=DD.DD.FAIL
        self.PASS=DD.DD.PASS

        self.subProcessRun={}
        self.maxNbPROC= maxNbPROC
        self.runEnv=runEnv

        print(self.dirname,end="")

    def nameDir(self,i):
        return  os.path.join(self.dirname,"dd.run%i" % (i+1))

    def mkdir(self,i):
         os.mkdir(self.nameDir(i))
    def rmdir(self,i):
        shutil.rmtree(self.nameDir(i))

    def runOneSample(self,i):
        rundir= self.nameDir(i)

        self.subProcessRun[i]=runCmdAsync([self.runCmd, rundir],
                                          os.path.join(rundir,"dd.run"),
                                          self.runEnv)

    def cmpOneSample(self,i):
        rundir= self.nameDir(i)
        if self.subProcessRun[i]!=None:
            getResult(self.subProcessRun[i])
        retval = runCmd([self.cmpCmd, self.refDir, rundir],
                        os.path.join(rundir,"dd.compare"))

        with open(os.path.join(self.dirname, rundir, "returnVal"),"w") as f:
            f.write(str(retval))
        if retval != 0:
            print("FAIL(%d)" % i)
            return self.FAIL
        else:
            return self.PASS

    def sampleToComputeToGetFailure(self, nbRun):
        """Return the list of samples which have to be computed to perforn nbRun Success run : None mean Failure [] Mean Success """
        listOfDir=[runDir for runDir in os.listdir(self.dirname) if runDir.startswith("dd.run")]
        done=[]
        for runDir in listOfDir:
            status=int((open(os.path.join(self.dirname, runDir, "returnVal")).readline()))
            if status!=0:
                return None
            done+=[runDir]

        res= [x for x in range(nbRun) if not ('dd.run'+str(x+1)) in done]
        return res

    def run(self):
        workToDo=self.sampleToComputeToGetFailure(self.nbRun)
        if workToDo==None:
            print(" --(cache) -> FAIL")
            return self.FAIL

        if len(workToDo)!=0:
            print(" --( run )-> ",end="",flush=True)

            if self.maxNbPROC==None:
                returnVal=self.runSeq(workToDo)
            else:
                returnVal=self.runPar(workToDo)

            if(returnVal==self.PASS):
                print("PASS(+" + str(len(workToDo))+"->"+str(self.nbRun)+")" )
            return returnVal
        print(" --(cache)-> PASS("+str(self.nbRun)+")")
        return self.PASS

    def runSeq(self,workToDo):

        for run in workToDo:
            self.mkdir(run)
            self.runOneSample(run)
            retVal=self.cmpOneSample(run)

            if retVal=="FAIL":
                return self.FAIL
        return self.PASS

    def runPar(self,workToDo):

        for run in workToDo:
            self.mkdir(run)
            self.runOneSample(run)
        for run in workToDo:
            retVal=self.cmpOneSample(run)

            if retVal=="FAIL":
                return self.FAIL

        return self.PASS


def md5Name(deltas):
    copyDeltas=copy.copy(deltas)
    copyDeltas.sort()
    return hashlib.md5(("".join(copyDeltas)).encode('utf-8')).hexdigest()


def prepareOutput(dirname):
     shutil.rmtree(dirname, ignore_errors=True)
     os.makedirs(dirname)



def failure():
    sys.exit(42)




def symlink(src, dst):
    if os.path.lexists(dst):
        os.remove(dst)
    os.symlink(src, dst)


class DDStoch(DD.DD):
    def __init__(self, config, prefix):
        DD.DD.__init__(self)
        self.config_=config
        self.run_ =  self.config_.get_runScript()
        self.compare_ = self.config_.get_cmpScript()
        self.cache_outcomes = False
        self.index=0
        self.prefix_ = os.path.join(os.getcwd(),prefix)
        self.ref_ = os.path.join(self.prefix_, "ref")

        prepareOutput(self.ref_)
        self.reference()
        self.mergeList()
        self.checkReference()


    def mergeList(self):
        """merge the file name.$PID into a uniq file called name """
        dirname=self.ref_
        name=self.getDeltaFileName()

        listOfExcludeFile=[ x for x in os.listdir(dirname) if self.isFileValidToMerge(x) ]
        if len(listOfExcludeFile)<1:
            print("The generation of exclusion/source files failed")
            failure()

        with open(os.path.join(dirname,listOfExcludeFile[0]), "r") as f:
                excludeMerged=f.readlines()

        for excludeFile in listOfExcludeFile[1:]:
            with open(os.path.join(dirname,excludeFile), "r") as f:
                for line in f.readlines():
                    if line not in excludeMerged:
                        excludeMerged+=[line]
        with open(os.path.join(dirname, name), "w" )as f:
            for line in excludeMerged:
                f.write(line)


    def checkReference(self):
        retval = runCmd([self.compare_,self.ref_, self.ref_],
                        os.path.join(self.ref_,"checkRef"))
        if retval != 0:
            print("FAILURE: the reference is not valid ")
            print("Suggestions:")
            print("\t1) check the correctness of the %s script"%self.compare_)

            print("Files to analyze:")
            print("\t run output: " +  os.path.join(self.ref_,"dd.out") + " " + os.path.join(self.ref_,"dd.err"))
            print("\t cmp output: " +  os.path.join(self.ref_,"checkRef.out") + " "+ os.path.join(self.ref_,"checkRef.err"))
            failure()

    def testWithLink(self, deltas, linkname):
        #by default the symlinks are generated when the test fail
        testResult=self._test(deltas)
        dirname = os.path.join(self.prefix_, md5Name(deltas))
        symlink(dirname, os.path.join(self.prefix_,linkname))
        return testResult

    def report_progress(self, c, title):
        if not self.config_.get_quiet:
            super().report_progress(c,title)

    def configuration_found(self, kind_str, delta_config,verbose=True):
        if verbose:
            print("%s (%s):"%(kind_str,self.coerce(delta_config)))
        self.testWithLink(delta_config, kind_str)

    def run(self, deltas=None):
        if deltas==None:
            deltas=self.getDelta0()

        algo=self.config_.get_ddAlgo()
        resConf=None
        if algo=="rddmin":
            resConf = self.RDDMin(deltas, self.config_.get_nbRUN())
        if algo.startswith("srddmin"):
            resConf= self.SRDDMin(deltas, self.config_.get_rddMinTab())
        if algo.startswith("drddmin"):
            resConf = self.DRDDMin(deltas,
                                   self.config_.get_rddMinTab(),
                                   self.config_.get_splitTab(),
                                   self.config_.get_splitGranularity())
        if algo=="ddmax":
            resConf= self.DDMax(deltas)
        else:
            if resConf!=None:
                flatRes=[c  for conf in resConf for c in conf]
                cmp= [delta for delta in deltas if  delta not in flatRes ]
                self.configuration_found("rddmin-cmp", cmp)

        return resConf

    def DDMax(self, deltas):
        res=self.interflop_dd_max(deltas)
        cmp=[delta for delta in deltas if delta not in res]
        self.configuration_found("ddmax", cmp)
        self.configuration_found("ddmax-cmp", res)

        return cmp

    def RDDMin(self, deltas,nbRun):
        ddminTab=[]
        testResult=self._test(deltas)
        if testResult!=self.FAIL:
            self.deltaFailedMsg(deltas)

        while testResult==self.FAIL:
            conf = self.interflop_dd_min(deltas,nbRun)

            ddminTab += [conf]
            self.configuration_found("ddmin%d"%(self.index), conf)
            #print("ddmin%d (%s):"%(self.index,self.coerce(conf)))

            #update deltas
            deltas=[delta for delta in deltas if delta not in conf]
            testResult=self._test(deltas,nbRun)
            self.index+=1
        return ddminTab

    def splitDeltas(self, deltas,nbRun,granularity):
        if self._test(deltas, self.config_.get_nbRUN())==self.PASS:
            return [] #short exit

        res=[] #result : set of smallest (each subset with repect with granularity lead to success)

        #two lists which contain tasks
        # -the fail status is known
        toTreatFailed=[deltas]
        # -the status is no not known
        toTreatUnknown=[]

        #name for progression
        algo_name="splitDeltas"

        def treatFailedCandidat(candidat):
            #treat a failing configuration
            self.report_progress(candidat, algo_name)

            # create subset
            cutSize=min(granularity, len(candidat))
            ciTab=self.split(candidat, cutSize)

            cutAbleStatus=False
            for i in range(len(ciTab)):
                ci=ciTab[i]
                #test each subset
                status=self._test(ci ,nbRun)
                if status==self.FAIL:
                    if len(ci)==1:
                        #if the subset size is one the subset is a valid ddmin : treat as such
                        self.configuration_found("ddmin%d"%(self.index), ci)
                        #print("ddmin%d (%s):"%(self.index,self.coerce(ci)))
                        self.index+=1
                        res.append(ci)
                    else:
                        #insert the subset in the begin of the failed task list
                        toTreatFailed.insert(0,ci)
                        #insert the remaining subsets to the unknown task list
                        tail= ciTab[i+1:]
                        tail.reverse() # to keep the same order
                        for cip in tail:
                            toTreatUnknown.insert(0,cip)
                        return
                    cutAbleStatus=True
            #the failing configuration is failing
            if cutAbleStatus==False:
                res.append(candidat)

        def treatUnknownStatusCandidat(candidat):
            #test the configuration : do nothing in case of success and add to the failed task list in case of success
            self.report_progress(candidat, algo_name+ "(unknownStatus)")
            status=self._test(candidat, nbRun)
            if status==self.FAIL:
                toTreatFailed.insert(0,candidat)
            else:
                pass

        # loop over tasks
        while len(toTreatFailed)!=0 or len(toTreatUnknown)!=0:

            unknownStatusSize=len(deltas) #to get a max
            if len(toTreatUnknown)!=0:
                unknownStatusSize=len(toTreatUnknown[0])

            if len(toTreatFailed)==0:
                treatUnknownStatusCandidat(toTreatUnknown[0])
                toTreatUnknown=toTreatUnknown[1:]
                continue

            #select the smallest candidat : in case of equality select a fail
            toTreatCandidat=toTreatFailed[0]
            if  len(toTreatCandidat) <= unknownStatusSize:
                cutCandidat=toTreatCandidat
                toTreatFailed=toTreatFailed[1:]
                treatFailedCandidat(cutCandidat)
            else:
                treatUnknownStatusCandidat(toTreatUnknown[0])
                toTreatUnknown=toTreatUnknown[1:]
        return res

    def SsplitDeltas(self, deltas, runTab, granularity):#runTab=splitTab ,granularity=2):
        #apply splitDeltas recussivly with increasing sample number (runTab)
        #remarks the remain treatment do not respect the binary split structure

        #name for progression
        algo_name="ssplitDelta"

        currentSplit=[deltas]
        for run in runTab:
            nextCurrent=[]
            for candidat in currentSplit:
                if len(candidat)==1:
                    nextCurrent.append(candidat)
                    continue
                self.report_progress(candidat,algo_name)
                res=self.splitDeltas(candidat,run, granularity)
                nextCurrent.extend(res)

            #the remainDeltas in recomputed from the wall list (indeed the set can increase with the apply )
            flatNextCurrent=[flatItem  for nextCurrentItem in nextCurrent for flatItem in nextCurrentItem]
            remainDeltas=[delta for delta in deltas if delta not in flatNextCurrent ]

            #apply split to remainDeltas
            self.report_progress(remainDeltas,algo_name)
            nextCurrent.extend(self.splitDeltas(remainDeltas, run, granularity))

            currentSplit=nextCurrent

        return currentSplit

    def DRDDMin(self, deltas, SrunTab, dicRunTab, granularity):#SrunTab=rddMinTab, dicRunTab=splitTab, granularity=2):
        #name for progression
        algo_name="DRDDMin"

        #assert with the right nbRun number
        nbRun=SrunTab[-1]
        testResult=self._test(deltas,nbRun)
        if testResult!=self.FAIL:
            self.deltaFailedMsg(deltas)

        #apply dichotomy
        candidats=self.SsplitDeltas(deltas,dicRunTab, granularity)
        print("Dichotomy split done")

        res=[]
        for candidat in candidats:
            if len(candidat)==1: #is a valid ddmin
                res+=[candidat]
                deltas=[delta for delta in deltas if delta not in candidat]
            else:
                self.report_progress(candidat, algo_name)
                #we do not known id candidat is a valid ddmin (in case of sparse pattern)
                resTab=self.SRDDMin(candidat,SrunTab)
                for resMin in resTab:
                    res+=[resMin] #add to res
                    deltas=[delta for delta in deltas if delta not in resMin] #reduce search space
        print("Dichotomy split analyze done")

        #after the split filter a classic (s)rddmin is applied
        testResult=self._test(deltas,nbRun)
        if testResult!=self.FAIL:
            return res
        else:
            return res+self.SRDDMin(deltas, SrunTab)



    def SRDDMin(self, deltas,runTab):#runTab=rddMinTab):
        #name for progression
        algo_name="SRDDMin"
        #assert with the right nbRun number
        nbRun=runTab[-1]
        testResult=self._test(deltas,nbRun)
        if testResult!=self.FAIL:
            self.deltaFailedMsg(deltas)

        ddminTab=[]

        #increasing number of run
        for run in runTab:
            testResult=self._test(deltas,run)

            #rddmin loop
            while testResult==self.FAIL:
                self.report_progress(deltas, algo_name)
                conf = self.interflop_dd_min(deltas,run)
                if len(conf)!=1:
                    #may be not minimal due to number of run)
                    for runIncValue in [x for x in runTab if x>run ]:
                        conf = self.interflop_dd_min(conf,runIncValue)
                        if len(conf)==1:
                            break

                ddminTab += [conf]
                self.configuration_found("ddmin%d"%(self.index), conf)
                #print("ddmin%d (%s):"%(self.index,self.coerce(conf)))
                self.index+=1
                #update search space
                deltas=[delta for delta in deltas if delta not in conf]
                #end test loop of rddmin
                testResult=self._test(deltas,nbRun)

        return ddminTab

    #Error Msg
    def deltaFailedMsg(self,delta):
        print("FAILURE: nothing to debug (the run with all symbols activated succeed)")
        print("Suggestions:")
        print("\t1) check the correctness of the %s script : the failure criteria may be too large"%self.compare_)
        print("\t2) check if the number of samples INTERFLOP_DD_NRUNS is sufficient ")

        dirname = md5Name(delta)
        print("Directory to analyze: %s"%dirname)
        failure()

    def allDeltaFailedMsg(self,deltas):
        print ("FAILURE: when interflop perturbs all parts of the program, its output is still detected as stable.")
        print ("Suggestions:")
        print ("\t1) check if the number of samples INTERFLOP_DD_NRUNS is sufficient")
        print ("\t2) check the correctness of the %s script : the failure criteria may be too large"%self.compare_)
        print ("\t3) set the env variable INTERFLOP_DD_UNSAFE : be careful it is realy unsafe")

        dirname = md5Name(delta)
        print("Directory to analyze: %s"%dirname)
        failure()


    def noDeltaSucceedMsg(self,deltas=[]):
        print("FAILURE: the comparison between interflop with activated symbols in nearest mode (ref) and interflop without activated symbols failed")

        print("Suggestions:")
        print("\t1) check if reproducibilty discrepancies are larger than the failure criteria of the script %s"%self.compare_)
        failure()

    def reference(self):
        retval = runCmd([self.run_, self.ref_],
                        os.path.join(self.ref_,"dd"),
                        self.referenceRunEnv())
        assert retval == 0, "Error during reference run"

    def getDelta0(self):
        with open(os.path.join(self.ref_ ,self.getDeltaFileName()), "r") as f:
            return f.readlines()


    def genExcludeIncludeFile(self, dirname, deltas, include=False, exclude=False):
        """Generate the *.exclude and *.include file in dirname rep from deltas"""
        excludes=self.getDelta0()
        dd=self.getDeltaFileName()

        if include:
            with open(os.path.join(dirname,dd+".include"), "w") as f:
                for d in deltas:
                    f.write(d)

        if exclude:
            with open(os.path.join(dirname,dd+".exclude"), "w") as f:
                for d in deltas:
                    excludes.remove(d)

                for line in excludes:
                    f.write(line)


    def _test(self, deltas,nbRun=None):
        if nbRun==None:
            nbRun=self.config_.get_nbRUN()

        dirname=os.path.join(self.prefix_, md5Name(deltas))
        if not os.path.exists(dirname):
            os.makedirs(dirname)
            self.genExcludeIncludeFile(dirname, deltas, include=True, exclude=True)

        vT=InterflopTask(dirname, self.ref_, self.run_, self.compare_ ,nbRun, self.config_.get_maxNbPROC() , self.sampleRunEnv(dirname))

        return vT.run()

