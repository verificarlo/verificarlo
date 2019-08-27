import os
import math
import sys

def exponentialRange(nbRun):
    tab=[int(nbRun / (2**i)) for i in range(1+int(math.floor(math.log(nbRun,2)))) ]
    tab.reverse()
    return tab


class ddConfig:
    def __init__(self, argv, environ,config_keys=["INTERFLOP"]):
        self.defaultValue()
        self.config_keys=config_keys
        self.parseArgv(argv)
        for config_key in self.config_keys:
            self.read_environ(environ, config_key)

    def defaultValue(self):
        self.nbRUN=5
        self.maxNbPROC=None
        self.ddAlgo="rddmin"
        self.rddminVariant="d"
        self.param_rddmin_tab="exp"
        self.param_dicho_tab="half"
        self.splitGranularity=2
        self.ddSym=False
        self.ddQuiet=False

    def parseArgv(self,argv):
        if "-h" in argv or "--help" in argv:
            print(self.get_EnvDoc(self.config_keys[-1]))
            self.failure()

        if len(argv)!=3:
            self.usageCmd()
            self.failure()

        self.runScript=self.checkScriptPath(argv[1])
        self.cmpScript=self.checkScriptPath(argv[2])

    def usageCmd(self):
        print("Usage: "+ sys.argv[0] + " runScript cmpScript")

    def failure(self):
        sys.exit(42)

    def checkScriptPath(self,fpath):
        if os.path.isfile(fpath) and os.access(fpath, os.X_OK):
            return os.path.abspath(fpath)
        else:
            print("Invalid Cmd:"+str(sys.argv))
            print(fpath + " should be executable")
            self.usageCmd()
            self.failure()

    def get_runScript(self):
        return self.runScript

    def get_cmpScript(self):
        return self.cmpScript

    def read_environ(self,environ, PREFIX):
        self.environ=environ #configuration to prepare the call to readOneOption
        self.PREFIX=PREFIX
        self.readOneOption("nbRUN", "int", "DD_NRUNS")
        self.readOneOption("maxNbPROC", "int", "DD_NUM_THREADS")
        if self.maxNbPROC!=None:
            if self.maxNbPROC < self.nbRUN:
                print("Due due implementation limitation (nbRun <=maxNbPROC or maxNbPROC=1): maxNbPROC unset\n")
                self.maxNbPROC=None

        self.readOneOption("ddAlgo", "string", "DD_ALGO", ["ddmax", "rddmin"])

        self.readOneOption("rddminVariant", "string","DD_RDDMIN", ["s", "stoch", "d", "dicho", "", "strict"])
        if self.rddminVariant=="stoch":
            self.rddminVariant="s"
        if self.rddminVariant=="dicho":
            self.rddminVariant="d"
        if self.rddminVariant=="strict":
            self.rddminVariant=""

        self.readOneOption("param_rddmin_tab", "string", "DD_RDDMIN_TAB", ["exp", "all", "single"])
        self.readOneOption("param_dicho_tab", "int/string", "DD_DICHO_TAB" , ["exp", "all", "half", "single"])
        self.readOneOption("splitGranularity", "int", "DD_DICHO_GRANULARITY")
        self.readOneOption("ddSym", "bool", "DD_SYM")
        self.readOneOption("ddQuiet", "bool", "DD_QUIET")

    def readOneOption(self,attribut,conv_type ,key_name, acceptedValue=None):
        value=False
        try:
            if conv_type=="int":
                value = int(self.environ[self.PREFIX+"_"+key_name])
            else:
                value = self.environ[self.PREFIX+"_"+key_name]

            if conv_type=="bool":
                value=True

            if acceptedValue==None :
                exec("self."+attribut+"= value")
            else:
                if value in acceptedValue:
                    exec("self."+attribut+"= value")
                elif conv_type=="string/int":
                    try:
                        exec("self."+attribut+"= int(value)")
                    except:
                        print("Error : "+ self.PREFIX+"_"+key_name+ " should be in "+str(acceptedValue) +" or be a int value")
                else:
                    print("Error : "+ self.PREFIX+"_"+key_name+ " should be in "+str(acceptedValue))
                    self.failure()
            exec("self."+attribut+"= value")
        except KeyError:
            pass


    def get_SymOrLine(self):
        if self.ddSym:
            return "sym"
        else:
            return "line"

    def get_splitGranularity(self):
        return self.splitGranularity

    def get_ddAlgo(self):
        if self.ddAlgo.endswith("rddmin"):
            return self.rddminVariant+self.ddAlgo
        return self.ddAlgo

    def get_maxNbPROC(self):
        return self.maxNbPROC

    def get_nbRUN(self):
        return self.nbRUN

    def get_quiet(self):
        return self.ddQuiet

    def get_rddMinTab(self):
        rddMinTab=None
        if self.param_rddmin_tab=="exp":
            rddMinTab=exponentialRange(self.nbRUN)
        if self.param_rddmin_tab=="all":
            rddMinTab=range(1,self.nbRUN+1)
        if self.param_rddmin_tab=="single":
            rddMinTab=[self.nbRUN]
        return rddMinTab

    def get_splitTab(self):
        splitTab=None
        if self.param_dicho_tab=="exp":
            splitTab=exponentialRange(self.nbRUN)
        if self.param_dicho_tab=="all":
            splitTab=range(self.nbRUN)
        if self.param_dicho_tab=="single":
            splitTab=[self.nbRUN]
        if self.param_dicho_tab=="half":
            splitTab=[ int(math.ceil(self.nbRUN / 2.))]
        if self.param_dicho_tab in [str(i) for i in range(1, self.nbRUN+1) ]:
            splitTab=[self.param_dicho_tab]
        return splitTab


    def get_EnvDoc(self,PREFIX="INTERFLOP"):
        doc="""List of environnment variables:
        PREFIXENV_DD_NRUNS : int (default:5)
        PREFIXENV_DD_NUM_THREADS : int (default None)
        PREFIXENV_DD_ALGO : in ["ddmax", "rddmin"] (default "rddmin")
        PREFIXENV_DD_RDDMIN : in ["s", "stoch", "dicho" ,"d", "strict",""] (default "d")
        PREFIXENV_DD_RDDMIN_TAB : in ["exp", "all" ,"single"] (default "exp")
        PREFIXENV_DD_DICHO_TAB : in ["exp", "all" ,"single", "half"] or int (default "half")
        PREFIXENV_DD_DICHO_GRANULARITY : int
        PREFIXENV_DD_QUIET : set or not (default not)
        PREFIXENV_DD_SYM : set or not (default not)
        """
        return doc.replace("PREFIXENV_",PREFIX+"_")
