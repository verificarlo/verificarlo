#!/usr/bin/python3

import os
import os.path
import subprocess
import shlex
import warnings
import sys


def sortingFunctionOutputFile(x):
	y = x.split(' ')[-1]
	y = y.split('=')[-1].strip()
	return int(y)


def main():
    
	run1_name = sys.argv[1]
	run2_name = sys.argv[2]

	#check if the reference run file exists
	if (os.path.exists(run1_name) is False):
		exit("run 1 file {} does not exist!".format(run1_name))
	run1_file = open(run1_name, "r")

	#check if the current run file exists
	if (os.path.exists(run2_name) is False):
		exit("current run file {} does not exist!".format(run2_name))
	run2_file = open(run2_name, "r")
    
    #parse the two files
	run1_lines = []
	for line in run1_file:
		run1_lines = run1_lines + [line]
	run2_lines = []
	for line in run2_file:
		run2_lines = run2_lines + [line]
 
	#close the files
	run1_file.close()
	run2_file.close()
 
	#sort the two files by their thread id
	run1_lines.sort(reverse=True, key=sortingFunctionOutputFile)
	run2_lines.sort(reverse=True, key=sortingFunctionOutputFile)
 
	#go through each line in the two files and check if the results
	#	of the computations match
	for line1, line2 in zip(run1_lines, run2_lines):
		res1 = line1.split(',')[2].strip()
		res2 = line2.split(',')[2].strip()
		if (res1 != res2):
			return 1
    
	return 0


if "__main__" == __name__:
	sys.exit(main())