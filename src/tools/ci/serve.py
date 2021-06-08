#############################################################################
#                                                                           #
#  This file is part of Verificarlo.                                        #
#                                                                           #
#  Copyright (c) 2015-2021                                                  #
#     Verificarlo contributors                                              #
#     Universite de Versailles St-Quentin-en-Yvelines                       #
#     CMLA, Ecole Normale Superieure de Cachan                              #
#                                                                           #
#  Verificarlo is free software: you can redistribute it and/or modify      #
#  it under the terms of the GNU General Public License as published by     #
#  the Free Software Foundation, either version 3 of the License, or        #
#  (at your option) any later version.                                      #
#                                                                           #
#  Verificarlo is distributed in the hope that it will be useful,           #
#  but WITHOUT ANY WARRANTY; without even the implied warranty of           #
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            #
#  GNU General Public License for more details.                             #
#                                                                           #
#  You should have received a copy of the GNU General Public License        #
#  along with Verificarlo.  If not, see <http://www.gnu.org/licenses/>.     #
#                                                                           #
#############################################################################

# Server for the Verificarlo CI report. This is simply a wrapper to avoid
# calling Bokeh directly.

import os
import calendar
import time
import json


def run(
        directory,
        show,
        port,
        allow_origin,
        logo_url,
        max_files,
        ignore_recent):
    '''Entry point of vfc_ci serve'''

    # Prepare arguments
    directory = "directory %s" % directory
    show = "--show" if show else ""
    logo = "logo %s" % logo_url if logo_url else ""
    max_files = "max_files %s" % max_files if max_files else ""
    ignore_recent = "ignore_recent %s" % ignore_recent if ignore_recent else ""

    dirname = os.path.dirname(__file__)

    # Call the "bokeh serve" command on the system
    command = "bokeh serve %s/vfc_ci_report %s --allow-websocket-origin=%s:%s --port %s --args %s %s %s %s" \
        % (dirname, show, allow_origin, port, port, directory, logo, max_files, ignore_recent)
    command = os.path.normpath(command)

    os.system(command)
