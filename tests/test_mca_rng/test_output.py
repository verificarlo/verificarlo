#!/usr/bin/env python3

import os
import os.path
import sys
from collections import Counter


def main():

    run1_name = sys.argv[1]
    run2_name = sys.argv[2]

    # check if the reference run file exists
    if os.path.exists(run1_name) is False:
        sys.exit(f"run 1 file {run1_name} does not exist!")

    # check if the current run file exists
    if os.path.exists(run2_name) is False:
        sys.exit("current run file {run2_name} does not exist!")

    # parse the two files
    run1_lines = []
    with open(run1_name, "r", encoding='utf-8') as run1_file:
        for line in run1_file:
            run1_lines = run1_lines + [line]
    run2_lines = []
    with open(run2_name, "r", encoding='utf-8') as run2_file:
        for line in run2_file:
            run2_lines = run2_lines + [line]

    # the two lists should have the same length
    if len(run1_lines) != len(run2_lines):
        return 1

    # generate a simplified version of the two lists, which only contain
    # the results of the computations
    run1_lines_simple = [(elem.split()[2]).strip() for elem in run1_lines]
    run2_lines_simple = [(elem.split()[2]).strip() for elem in run2_lines]

    # check if the outputs of the two runs contain the same elements,
    # not necessarily in the same order
    run1_lines_count = Counter(run1_lines_simple)
    run2_lines_count = Counter(run2_lines_simple)
    if run1_lines_count != run2_lines_count:
        print(run1_lines_count - run2_lines_count)
        return 1

    return 0


if "__main__" == __name__:
    sys.exit(main())
