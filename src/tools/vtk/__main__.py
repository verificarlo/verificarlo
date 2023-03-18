#!/usr/bin/env python3

#############################################################################
#                                                                           #
#  This file is part of the Verificarlo project,                            #
#  under the Apache License v2.0 with LLVM Exceptions.                      #
#  SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception.                 #
#  See https://llvm.org/LICENSE.txt for license information.                #
#                                                                           #
#                                                                           #
#  Copyright (c) 2015                                                       #
#     Universite de Versailles St-Quentin-en-Yvelines                       #
#     CMLA, Ecole Normale Superieure de Cachan                              #
#                                                                           #
#  Copyright (c) 2018                                                       #
#     Universite de Versailles St-Quentin-en-Yvelines                       #
#                                                                           #
#  Copyright (c) 2019-2021                                                  #
#     Verificarlo Contributors                                              #
#                                                                           #
#############################################################################


import argparse
import io
import os
import shutil
import sys
import xml.etree.ElementTree as ET
from contextlib import closing

import numpy as np


def error(code, msg, **kwargs):
    """Fails with an error message. Supports format like keyword arguments"""
    print(msg.format(**kwargs))
    sys.exit(code)


def find_dirs(path):
    """Returns the direct directories of path"""
    for i in os.listdir(path):
        fpath = os.path.join(path, i)
        if os.path.isdir(fpath):
            yield fpath


def find_vtu(path):
    """Returns the .vtu files inside path"""
    for i in os.listdir(path):
        if i.endswith(".vtu"):
            yield i


def np_to_string(a):
    """Converts a numpy double array to a string"""
    with closing(io.StringIO()) as s:
        np.savetxt(s, a, "%.18g")
        return "\n" + s.getvalue()


def merge_vtu(output_vtu, vfc_dirs, vtu_filename, args):
    """Postprocess a set of .vtu files adding verificarlo
    accuracy information.
     output_vtu: vtu that will be enriched with accuracy information
     vfc_dirs: set of directories that contain the different outputs
     vtu_filename: basename of the vtu file
     args: configuration arguments
    """
    # open output file
    output = ET.parse(output_vtu)

    # find names of float32 DataArray nodes
    names = []
    for c in output.findall(".//DataArray[@type='Float32']"):
        names.append(c.get("Name"))

    for name in names:
        rows = []
        xpath = ".//DataArray[@type='Float32'][@Name='{}']".format(name)

        # for each verificarlo trace extract data as a numpy double array
        for d in vfc_dirs:
            vtu_path = os.path.join(d, vtu_filename)
            inp = ET.parse(vtu_path)
            c = inp.find(xpath)
            assert c is not None
            rows.append(np.fromstring(c.text, np.float64, sep=" "))

        # compute the mean and significant digits across MCA traces
        mat = np.stack(rows)
        mean = np.mean(mat, axis=0)
        std = np.std(mat, axis=0)

        if args.std:
            s = std
        else:
            s = -np.log10(std / mean)
            s[np.isnan(s)] = 0.0
            s[s < 0.0] = 0.0
            s[s > 17.0] = 17.0

        # original element
        original = output.find(xpath)
        accuracy = original.copy()

        # update the output_vtu with the mean
        if args.mean:
            original.text = np_to_string(mean)

        # create an accuracy node with the accuracy
        accuracy.set("Name", name + "_vfc_accuracy")
        accuracy.text = np_to_string(s)

        # retrieve parent
        parent = output.find(xpath + "/..")
        if parent.tag != "Points":
            insert_in = parent
        else:
            insert_in = output.find(".//PointData")

        # insert it as a sibling
        insert_in.insert(0, accuracy)

    # write back the output file
    output.write(output_vtu)


def postprocess(verificarlo_dir, output_dir, args):
    """postprocess a verificarlo output directory"""

    # find the list of verificarlo traces directories
    dirs = list(find_dirs(verificarlo_dir))
    if not dirs:
        error(
            2, "The directory {dir} contains no verificarlo traces", dir=verificarlo_dir
        )

    # ensure that the output directory does not exists
    if os.path.exists(output_dir):
        error(3, "The output directory {dir} already exists", dir=output_dir)

    # Prepare reference output
    if args.r:
        shutil.copyfile(args.r, output_dir)
    else:
        # Use first vtu file as base for the merge
        shutil.copytree(dirs[0], output_dir)

    # find the list of vtu files in first verificarlo trace
    vtus = [os.path.basename(f) for f in find_vtu(dirs[0])]

    for vtu in vtus:
        output_vtu = os.path.join(output_dir, vtu)
        print("merging " + output_vtu)
        merge_vtu(output_vtu, dirs, vtu, args)


def main():
    parser = argparse.ArgumentParser(
        description="Postprocess a set of VTK verificarlo outputs"
    )
    parser.add_argument(
        "-o",
        metavar="OUTPUT_DIR",
        default="vfc_postprocess",
        help="write postprocessed files to <OUTPUT_DIR>",
    )
    parser.add_argument(
        "INPUT_DIR",
        help="input directory that contains a set of VTK outputs "
        + "generated with verificarlo. Each output should be in "
        + "a separate directory, such as INPUT_DIR/output01, "
        + " INPUT_DIR/output02, etc.",
    )
    parser.add_argument(
        "-r",
        metavar="REFERENCE_OUTPUT",
        help="the reference output that will be used in the "
        + "postprocess data. If not provided, the first "
        + "output directory found in INPUT_DIR will be used.",
    )
    parser.add_argument(
        "--std",
        action="store_true",
        help="report numerical accuracy as a standard deviation. "
        + "By default numerical accuracy is reported as the "
        + "number of significant decimal digits.",
    )
    parser.add_argument(
        "--mean",
        action="store_true",
        help="instead of using the reference output, each floating "
        + " point array is replaced by the mean of the verificarlo "
        + " outputs.",
    )

    args = parser.parse_args()
    postprocess(verificarlo_dir=args.INPUT_DIR, output_dir=args.o, args=args)


if __name__ == "__main__":
    main()
