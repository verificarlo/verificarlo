##############################################################################\
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

# Server for the Verificarlo CI report. This is simply a wrapper to avoid
# calling Bokeh directly.

import os


def run(directory, show, port, allow_origin, logo_url, max_files, ignore_recent):
    """Entry point of vfc_ci serve"""

    # Prepare arguments
    directory = "directory %s" % directory
    show = "--show" if show else ""
    logo = "logo %s" % logo_url if logo_url else ""
    max_files = "max_files %s" % max_files if max_files else ""
    ignore_recent = "ignore_recent %s" % ignore_recent if ignore_recent else ""

    dirname = os.path.dirname(__file__)

    # Call the "bokeh serve" command on the system

    # NOTE It's also possible to set this up manually, but would require to
    # re-write the Tornado server, which means setting up public directories,
    # generating the Jinja template, etc... => This is an option for a future
    # commit.

    command = (
        "bokeh serve %s/vfc_ci_report %s --allow-websocket-origin=%s:%s --port %s --args %s %s %s %s"
        % (
            dirname,
            show,
            allow_origin,
            port,
            port,
            directory,
            logo,
            max_files,
            ignore_recent,
        )
    )
    command = os.path.normpath(command)

    os.system(command)
