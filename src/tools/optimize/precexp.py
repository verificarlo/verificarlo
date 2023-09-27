#!/usr/bin/env python3

import argparse
import math
import os.path
import shlex
import subprocess
import sys

import pandas as pd

vfc_profile_file = "vfc_profile.txt"
vfc_config_file = "vfc_config.txt"
output_dir = ["vfc_ref", "vfc_std"]

vfc_maxTimeout = None
vfc_mode = None
vfc_vprec_mode = None


def set_environment_variable(envVar, value, env):
    """ replace current value, if the environment variable already exists """
    env[envVar.strip()] = value.strip()

    return env


def unset_environment_variable(envVar, env):
    """ remove environment variable, if it already exists """
    if (envVar.strip() in env):
        del env[envVar.strip()]

    return env


def add_environment_variable(envVar, value, env):
    """ check if the environment variable already exists, then append or add the new value """
    if (envVar.strip() in env):
        env[envVar.strip()] = value.strip() + ":" + env[envVar.strip()]
    else:
        env[envVar.strip()] = value.strip()

    return env


def shell(cmd, newEnv, maxTimeout=None):
    env = None
    if (newEnv is None):
        env = os.environ.copy()
    else:
        env = newEnv

    cmdNew = ""
    cmdSplit = shlex.split(cmd)
    for cmdEntry in cmdSplit:
        cmdNew = cmdNew + " " + cmdEntry.strip()

    proc = subprocess.Popen(cmdNew.split(),
                            stdout=subprocess.PIPE,
                            env=env)

    try:
        proc.wait(maxTimeout)
    except subprocess.TimeoutExpired:
        proc.terminate()
        return 42

    return proc.returncode


def run(script, output_dir, env, maxTimeout=None):
    """ create the runing command """
    command = "./" + script + " " + output_dir

    return_code = shell(command, env, maxTimeout)

    if return_code != 0:
        print("Error during run, return code =  {}".format(return_code))

    return return_code


def cmp(script, reference_dir, current_dir, env, maxTimeout=None):
    """ create the comparison command """
    command = "./" + script + " " + reference_dir + " " + current_dir

    return (shell(command, env, maxTimeout) == 0)


def getFunctionInfo(FunctionID):
    """ returns the information from the ID function """
    CallNb = int(FunctionID[FunctionID.rfind('/')+1:])
    FunctionID = FunctionID[:FunctionID.rfind('/')]
    Fline = int(FunctionID[FunctionID.rfind('/')+1:])
    FunctionID = FunctionID[:FunctionID.rfind('/')]
    Fname = FunctionID[FunctionID.rfind('/')+1:]
    FunctionID = FunctionID[:FunctionID.rfind('/')]
    Fparent = FunctionID[FunctionID.rfind('/')+1:]
    FunctionID = FunctionID[:FunctionID.rfind('/')]
    Ffile = FunctionID[FunctionID.rfind('/')+1:]

    return CallNb, Fline, Fparent, Fname, Ffile


def getProfile(file, function_list):
    """ get informations from profile file """
    if not os.path.isfile(file):
        print("error profile file not found")
        sys.exit()

    f = open(file, 'r')
    Lines = f.readlines()

    Functions = []
    Arguments = []

    i = 0

    # read the profile file function by function
    while i < len(Lines):
        ID, Lib, Int, Float, Double, Prec64, Range64, \
            Prec32, Range32, Ninputs, Noutputs, Ncalls = Lines[i].split(
                sep="\t")

        Lib = int(Lib)
        Int = int(Int)
        Float = int(Float)
        Double = int(Double)
        Prec64 = int(Prec64)
        Range64 = int(Range64)
        Prec32 = int(Prec32)
        Range32 = int(Range32)
        Ninputs = int(Ninputs)
        Noutputs = int(Noutputs)
        Ncalls = int(Ncalls)

        thisFunction = [ID, Lib, Int, Float, Double, Prec64, Range64,
                        Prec32, Range32, Ninputs, Noutputs, Ncalls]

        CallNb, Fline, Fparent, Fname, Ffile = getFunctionInfo(ID)

        # if the function or its parent is not in the function list skip it (only if the function list is not empty)
        if len(function_list) != 0 and (not Fname in function_list) and (not Fparent in function_list):
            i = i + Ninputs + Noutputs + 1
            continue

        Functions.append(thisFunction)

        j = i + 1
        while j < i + Ninputs + Noutputs + 1:
            IO, ArgID, Type, Prec, Range, Min, Max = Lines[j].split()

            Type = int(Type)
            Prec = int(Prec)
            Range = int(Range)
            Min = int(Min)
            Max = int(Max)

            functionArgs = [ID, ArgID, Lib, Int, IO,
                            Type, Prec, Range, Min, Max, Ncalls]

            Arguments.append(functionArgs)

            j = j + 1

        i = i + Ninputs + Noutputs + 1

    # creation of the dataframe for exploration of internal operations
    FunctionsFrame = pd.DataFrame(Functions,
                                  columns=['ID', 'Lib', 'Int', 'Float', 'Double', 'Prec64',
                                           'Range64', 'Prec32', 'Range32', 'Ninputs', 'Noutputs', 'Ncalls']).sort_values(['Ncalls', 'ID'], ascending=False).reset_index(drop=True)

    # index of functions compiled with verificarlo, used to explore internal operations
    FunctionsFilter = FunctionsFrame.copy()
    FunctionsFilter = FunctionsFilter[(
        FunctionsFilter['Lib'] == 0) & (FunctionsFilter['Int'] == 0)]

    # creation of the dataframe for exploration of arguments
    ArgumentsFrame = pd.DataFrame(Arguments,
                                  columns=['ID', 'ArgID', 'Lib', 'Int', 'IO', 'Type', 'Prec', 'Range', 'Min', 'Max', 'Ncalls']).sort_values(['Ncalls', 'ID'], ascending=False).reset_index(drop=True)

    f.close()

    return FunctionsFrame, FunctionsFilter.index, ArgumentsFrame


def save(Arguments, Operations, File):
    """ save in the given file the dataframes used for internal operations and arguments """
    f = open(File, 'w')

    arg_index = 0

    for i in Operations.index:
        f.write("{ID}\t{Lib}\t{Int}\t{Float}\t{Double}\t{Prec64}\t{Range64}\t{Prec32}\t{Range32}\t{Ninputs}\t{Noutputs}\t{Ncalls}\n".format(
            ID=Operations.at[i, "ID"],
            Lib=Operations.at[i, "Lib"],
            Int=Operations.at[i, "Int"],
            Float=Operations.at[i, "Float"],
            Double=Operations.at[i, "Double"],
            Prec64=Operations.at[i, "Prec64"],
            Range64=Operations.at[i, "Range64"],
            Prec32=Operations.at[i, "Prec32"],
            Range32=Operations.at[i, "Range32"],
            Ninputs=Operations.at[i, "Ninputs"],
            Noutputs=Operations.at[i, "Noutputs"],
            Ncalls=Operations.at[i, "Ncalls"]))

        j = arg_index
        while j < arg_index + Operations.at[i, "Ninputs"] + Operations.at[i, "Noutputs"]:
            f.write("{IO}\t{ArgID}\t{Type}\t{Prec}\t{Range}\n".format(
                IO=Arguments.at[j, 'IO'],
                ArgID=Arguments.at[j, 'ArgID'],
                Type=Arguments.at[j, 'Type'],
                Prec=Arguments.at[j, 'Prec'],
                Range=Arguments.at[j, 'Range']))
            j = j + 1
        arg_index = j

    f.close()


def Check(runPath, cmpPath, reference_dir, current_dir, config_file, env, Arguments, Operations):
    """ Save the current state of the exploration in a configuration file,
        execute the program, return the result of the comparison
    """
    save(Arguments, Operations, config_file)

    runRetCode = run(runPath, current_dir, env, vfc_maxTimeout)
    if (runRetCode != 0):
        return False

    return cmp(cmpPath, reference_dir, current_dir, env)


def dich_search_operations(index,
                           field,
                           l,
                           r,
                           runPath,
                           cmpPath,
                           reference_dir,
                           current_dir,
                           config_file,
                           env,
                           Arguments,
                           Operations):
    """ Dichotomic research to find the minimum precision for given field """

    Operations.at[index, field] = l

    if not Check(runPath, cmpPath, reference_dir, current_dir, config_file, env, Arguments, Operations):
        flag = 1
        while flag:
            p = math.floor((r + l) / 2)
            Operations.at[index, field] = p
            if Check(runPath, cmpPath, reference_dir, current_dir, config_file, env, Arguments, Operations):
                if l >= r:
                    flag = 0
                    break
                r = p
            else:
                l = p+1


def search_minimum_operations(runPath,
                              cmpPath,
                              reference_dir,
                              current_dir,
                              config_file,
                              env,
                              Arguments,
                              Operations,
                              index):
    """ Find the minimum precision for the internal operations of the given function """

    # if the function uses double precision operations
    if Operations.at[index, 'Double']:
        # minimize mantissa
        dich_search_operations(index, 'Prec64', 1, 52, runPath, cmpPath,
                               reference_dir, current_dir, config_file, env, Arguments, Operations)
        # minimize exponent
        dich_search_operations(index, 'Range64', 2, 11, runPath, cmpPath,
                               reference_dir, current_dir, config_file, env, Arguments, Operations)
    else:
        Operations.at[index, 'Prec64'] = 1
        Operations.at[index, 'Range64'] = 2

    # if the function uses simple precision operations
    if Operations.at[index, 'Float']:
        # minimize mantissa
        dich_search_operations(index, 'Prec32', 1, 23, runPath, cmpPath,
                               reference_dir, current_dir, config_file, env, Arguments, Operations)
        # minimize exponent
        dich_search_operations(index, 'Range32', 2, 8, runPath, cmpPath,
                               reference_dir, current_dir, config_file, env, Arguments, Operations)
    else:
        Operations.at[index, 'Prec32'] = 1
        Operations.at[index, 'Range32'] = 2


def dich_search_arguments(index,
                          field,
                          l,
                          r,
                          runPath,
                          cmpPath,
                          reference_dir,
                          current_dir,
                          config_file,
                          env,
                          Arguments,
                          Operations):
    """ Dichotomic research to find the minimum precision for given field """

    Arguments.at[index, field] = l

    # minimize mantissa
    if not Check(runPath, cmpPath, reference_dir, current_dir, config_file, env, Arguments, Operations):
        flag = 1
        while flag:
            p = math.floor((r + l) / 2)
            Arguments.at[index, field] = p
            if Check(runPath, cmpPath, reference_dir, current_dir, config_file, env, Arguments, Operations):
                if l >= r:
                    flag = 0
                    break
                r = p
            else:
                l = p+1


def search_minimum_arguments(runPath,
                             cmpPath,
                             reference_dir,
                             current_dir,
                             config_file,
                             env,
                             Arguments,
                             Operations,
                             index):
    """ Find the minimum precision for a given argument of a function """

    # If is a float or float ptr
    if Arguments.at[index, 'Type'] == 0 or Arguments.at[index, 'Type'] == 2:
        # minimize mantissa
        dich_search_arguments(index, 'Prec', 1, 23, runPath, cmpPath,
                              reference_dir, current_dir, config_file, env, Arguments, Operations)
        # minimize exponent
        dich_search_arguments(index, 'Range', 2, 8, runPath, cmpPath,
                              reference_dir, current_dir, config_file, env, Arguments, Operations)

    # If is a double or double ptr
    elif Arguments.at[index, 'Type'] == 1 or Arguments.at[index, 'Type'] == 3:
        # minimize mantissa
        dich_search_arguments(index, 'Prec', 1, 52, runPath, cmpPath,
                              reference_dir, current_dir, config_file, env, Arguments, Operations)
        # minimize exponent
        dich_search_arguments(index, 'Range', 2, 11, runPath, cmpPath,
                              reference_dir, current_dir, config_file, env, Arguments, Operations)


def main():
    parser = argparse.ArgumentParser(description="The vfc_precexp script tries "
                                     "to minimize the precision for each function "
                                     "call of a code compiled with verificarlo")
    parser.add_argument(
        "runPath", help="command used to execute the target program")
    parser.add_argument(
        "cmpPath", help="command used to compare an execution with the reference")

    parser.add_argument("--maxTimeout", type=int, default=None,
                        help="maximum execution time (ms)")
    parser.add_argument("-m", "--mode", choices=["arguments", "operations", "all"], default="all",
                        help="function instrumentation mode")
    parser.add_argument("--vprec_mode", choices=["ieee", "ib", "ob", "all"], default="ob",
                        help="vprec operating mode")

    args = parser.parse_known_args()

    if len(sys.argv) < 3:
        print("explore.py [runPath] [cmpPath] functions ...")
        sys.exit()

    runPath = args[0].runPath
    cmpPath = args[0].cmpPath

    vfc_vprec_mode = args[0].vprec_mode
    vfc_mode = args[0].mode
    if (args[0].maxTimeout != None):
        vfc_maxTimeout = int(args[0].maxTimeout)

    function_list = args[1]

    if not os.path.exists(runPath):
        print("runing script not found")
        sys.exit(0)

    if not os.path.exists(cmpPath):
        print("comparison script not found")
        sys.exit(0)

    if not os.access(runPath, os.X_OK):
        print("runing scirpt is not executable")
        sys.exit(0)

    if not os.access(cmpPath, os.X_OK):
        print("comparison scirpt is not executable")
        sys.exit(0)

    if os.path.isfile("./exp_ref_output"):
        os.mkdir("./exp_ref_output")

    if os.path.isfile("./exp_std_output"):
        os.mkdir("./exp_std_output")

    env = os.environ.copy()

    if not os.path.isdir(output_dir[0]):
        os.mkdir(output_dir[0])

    if not os.path.isdir(output_dir[1]):
        os.mkdir(output_dir[1])

    if not os.path.exists("vfc_exp_data/"):
        os.mkdir("vfc_exp_data")

    #########################################
    #  				Profile Run 			#
    #########################################
    # unset input file
    if "VFC_PREC_INPUT" in env:
        unset_environment_variable("VFC_PREC_INPUT", env)

    # set backend
    set_environment_variable(
        "VFC_BACKENDS", "libinterflop_vprec.so --prec-output-file={}".format(vfc_profile_file), env)

    # run to create profile file
    run(runPath, output_dir[0], env)

    # get dataset
    FunctionsFrame, FunctionsIndex, ArgumentsFrame = getProfile(
        vfc_profile_file, function_list)

    #########################################
    #  				Explore Operations 	    #
    #########################################
    if (vfc_mode == "operations"):
        print("-------------- Operations --------------")

        # set backend
        set_environment_variable(
            "VFC_BACKENDS", "libinterflop_vprec.so --prec-input-file={} --daz --ftz --mode={} --instrument=operations".format(vfc_config_file, vfc_vprec_mode), env)

        FunctionsFrameCopy = FunctionsFrame.copy()
        cpt = 1
        for i in FunctionsIndex:
            print(cpt, "/", len(FunctionsIndex))
            search_minimum_operations(runPath,
                                      cmpPath,
                                      output_dir[0],
                                      output_dir[1],
                                      vfc_config_file,
                                      env,
                                      ArgumentsFrame,
                                      FunctionsFrameCopy,
                                      i)
            cpt += 1

        FunctionsFrameCopy.to_csv("vfc_exp_data/OperationsResults.csv")

    #########################################
    #  				Explore Arguments 		#
    #########################################
    if (vfc_mode == "arguments"):
        print("-------------- Arguments --------------")

        # set backend
        set_environment_variable(
            "VFC_BACKENDS", "libinterflop_vprec.so --prec-input-file={} --daz --ftz --mode={} --instrument=arguments".format(vfc_config_file, vfc_vprec_mode), env)

        ArgumentsFrameCopy = ArgumentsFrame.copy()
        FunctionsFrameCopy = FunctionsFrame.copy()
        cpt = 1
        for i in range(len(FunctionsFrame.index)):
            tmp = ArgumentsFrameCopy[(
                ArgumentsFrameCopy['ID'] == FunctionsFrameCopy.at[i, 'ID'])]
            for j in tmp.index:
                print(cpt, "/", len(ArgumentsFrameCopy.index))
                search_minimum_arguments(runPath,
                                         cmpPath,
                                         output_dir[0],
                                         output_dir[1],
                                         vfc_config_file,
                                         env,
                                         ArgumentsFrameCopy,
                                         FunctionsFrameCopy,
                                         j)

                cpt += 1

        ArgumentsFrameCopy.to_csv("vfc_exp_data/ArgumentsResults.csv")

    #########################################
    #  				Explore All 			#
    #########################################
    if (vfc_mode == "all"):
        print("-------------- All --------------")

        # set backend
        set_environment_variable(
            "VFC_BACKENDS", "libinterflop_vprec.so --prec-input-file={} --daz --ftz --mode={} --instrument=all".format(vfc_config_file, vfc_vprec_mode), env)

        ArgumentsFrameCopy = ArgumentsFrame.copy()
        FunctionsFrameCopy = FunctionsFrame.copy()
        cpt = 1
        for i in range(len(FunctionsFrameCopy.index)):
            print(cpt, "/", len(FunctionsFrameCopy.index))
            tmp = ArgumentsFrameCopy[(
                ArgumentsFrameCopy['ID'] == FunctionsFrameCopy.at[i, 'ID'])]
            for j in tmp.index:
                search_minimum_arguments(runPath,
                                         cmpPath,
                                         output_dir[0],
                                         output_dir[1],
                                         vfc_config_file,
                                         env,
                                         ArgumentsFrameCopy,
                                         FunctionsFrameCopy,
                                         j)
            if i in FunctionsIndex:
                search_minimum_operations(runPath,
                                          cmpPath,
                                          output_dir[0],
                                          output_dir[1],
                                          vfc_config_file,
                                          env,
                                          ArgumentsFrameCopy,
                                          FunctionsFrameCopy,
                                          i)

            tmp = ArgumentsFrameCopy[(
                ArgumentsFrameCopy['ID'] == FunctionsFrameCopy.at[i, 'ID'])]

            cpt += 1

        ArgumentsFrameCopy.to_csv("vfc_exp_data/AllArgsResults.csv")
        FunctionsFrameCopy.to_csv("vfc_exp_data/AllOpsResults.csv")


if __name__ == "__main__":
    main()
