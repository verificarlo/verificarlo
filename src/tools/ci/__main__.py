#!/usr/bin/env python3

# \
#                                                                           #\
#  This file is part of the Verificarlo project,                            #\
#  under the Apache License v2.0 with LLVM Exceptions.                      #\
#  SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception.                 #\
#  See https://llvm.org/LICENSE.txt for license information.                #\
#                                                                           #\
#                                                                           #\
#  Copyright (c) 2015                                                       #\
#     Universite de Versailles St-Quentin-en-Yvelines                       #\
#     CMLA, Ecole Normale Superieure de Cachan                              #\
#                                                                           #\
#  Copyright (c) 2018                                                       #\
#     Universite de Versailles St-Quentin-en-Yvelines                       #\
#                                                                           #\
#  Copyright (c) 2019-2021                                                  #\
#     Verificarlo Contributors                                              #\
#                                                                           #\
#############################################################################

# This is the entry point of the Verificarlo CI command line interface, which is
# based on argparse and this article :
# https://mike.depalatis.net/blog/simplifying-argparse.html
# From here, 3 subcommands can be called :
# - setup : create a vfc_ci branch and workflow on the current Git repo
# - test : run and export test results according to the vfc_tests_config.json
# - serve : launch a Bokeh server to visualize run results

import argparse

##########################################################################

# Parameters validation helpers


def is_port(string):
    value = int(string)
    if value < 0 or value > 65535:
        raise argparse.ArgumentTypeError("Value has to be between 0 and 65535")
    return value


def is_directory(string):
    import os

    isdir = os.path.isdir(string)
    if not isdir:
        raise argparse.ArgumentTypeError("Directory does not exist")

    return string


def is_strictly_positive(string):
    value = int(string)
    if value <= 0:
        raise argparse.ArgumentTypeError("Value has to be strictly positive")
    return value


##########################################################################

# Subcommand decorator

cli = argparse.ArgumentParser(
    description="Define, run, automatize, and visualize custom Verificarlo tests."
)
subparsers = cli.add_subparsers(dest="subcommand")


def subcommand(description="", args=[], parent=subparsers):
    def decorator(func):
        parser = parent.add_parser(func.__name__, description=description)
        for arg in args:
            parser.add_argument(*arg[0], **arg[1])
        parser.set_defaults(func=func)

    return decorator


def argument(*name_or_flags, **kwargs):
    return ([*name_or_flags], kwargs)


##########################################################################

# "setup" subcommand


@subcommand(
    description="Create an automated workflow to execute Verificarlo tests.",
    args=[
        argument(
            "git_host",
            help="""
            specify where your repository is hosted
            """,
            choices=["github", "gitlab"],
        )
    ],
)
def setup(args):
    import verificarlo.ci.setup

    verificarlo.ci.setup.run(args.git_host)

    # "test" subcommand


@subcommand(
    description="Execute predefined Verificarlo tests and save their results.",
    args=[
        argument(
            "-g",
            "--is-git-commit",
            help="""
            When specified, the last Git commit of the local repository (working
            directory) will be fetched and associated with the run.
            """,
            action="store_true",
        ),
        argument(
            "-r",
            "--export-raw-results",
            help="""
            Specify if an additional HDF5 file containing the raw results must be
            exported.
            """,
            action="store_true",
        ),
        argument(
            "-d",
            "--dry-run",
            help="""
            Perform a dry run by not saving the test results.
            """,
            action="store_true",
        ),
    ],
)
def test(args):
    import verificarlo.ci.test

    verificarlo.ci.test.run(args.is_git_commit, args.export_raw_results, args.dry_run)

    # "serve" subcommand


@subcommand(
    description="""
    Start a server to visualize Verificarlo test results.
    """,
    args=[
        argument(
            "-d",
            "--data-directory",
            help="""
            Specify where to look for the run files.
            """,
            type=is_directory,
            default=".",
        ),
        argument(
            "-s",
            "--show",
            help="""
            Specify if the report must be opened in the browser at server
            startup.
            """,
            action="store_true",
        ),
        argument(
            "-p",
            "--port",
            help="""
            The port on which the server will run. Defaults to 8080.
            """,
            type=is_port,
            default=8080,
        ),
        argument(
            "-a",
            "--allow-origin",
            help="""
            The origin (URL) from which the report will be accessible.
            Port number must not be specified. Defaults to * (allow everything).
            """,
            type=str,
            default="*",
        ),
        argument(
            "-l",
            "--logo",
            help="""
            Specify the URL of an image to be displayed in the report header.
            """,
            type=str,
        ),
        argument(
            "-f",
            "--max-files",
            help="""
            Specify the maximum number of run files that can be integrated into
            the report to avoid performance issues. If the number of detected
            files exceeds this value, the most recent ones will be kept.
            Defaults to 100.
            """,
            type=is_strictly_positive,
        ),
        argument(
            "-i",
            "--ignore-recent",
            help="""
            Specify the number of most recent files that should be ignored.
            Combining this with the --max-files option makes it possible to
            select any chronological range of files. Defaults to 0.
            """,
            type=is_strictly_positive,
        ),
    ],
)
def serve(args):
    import verificarlo.ci.serve

    verificarlo.ci.serve.run(
        args.data_directory,
        args.show,
        args.port,
        args.allow_origin,
        args.logo,
        args.max_files,
        args.ignore_recent,
    )


###############################################################################
def main():
    args = cli.parse_args()
    if args.subcommand is None:
        cli.print_help()
    else:
        args.func(args)

    # Main command group and entry point


if __name__ == "__main__":
    main()
