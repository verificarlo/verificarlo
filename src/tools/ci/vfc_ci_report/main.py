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

# Look for and read all the run files in the current directory (ending with
# .vfcrun.h5), and lanch a Bokeh server for the visualization of this data.

import os
import sys
import time
import json

import pandas as pd
import numpy as np

from bokeh.plotting import curdoc
from bokeh.models import Select, CustomJS

# Local imports from vfc_ci_server
import compare_runs
import inspect_runs
import helper

##########################################################################


# Read server arguments
# (this is quite easy because Bokeh server is called through a wrapper, so
# we know exactly what the arguments might be)


has_logo = False
logo_url = ""

directory = "."
max_files = 100


for i in range(1, len(sys.argv)):

    # Look for a logo URL
    # If a logo URL is specified, it will be included in the report's header
    if sys.argv[i] == "logo":
        curdoc().template_variables["logo_url"] = sys.argv[i + 1]
        has_logo = True

    # By default, run files are read from ., look if specified othewise
    if sys.argv[i] == "directory":
        directory = sys.argv[i + 1]

    # By default, the n latest files are selected, but this can be modified
    if sys.argv[i] == "max_files":
        max_files = int(sys.argv[i + 1])

curdoc().template_variables["has_logo"] = has_logo


##########################################################################


# Read vfcrun files, and aggregate them in one dataset

run_files = [f for f in os.listdir(directory) if f.endswith(".vfcrun.h5")]

if len(run_files) == 0:
    print(
        "Warning [vfc_ci]: Could not find any vfcrun files in the directory. "
        "This will result in server errors and prevent you from viewing the report.")

# These are arrays of Pandas dataframes for now
metadata = []
data = []

# First run for metadata
for f in run_files:
    path = os.path.normpath(directory + "/" + f)
    metadata.append(pd.read_hdf(path, "metadata"))
    current_metadata = pd.read_hdf(path, "metadata")

metadata = pd.concat(metadata).sort_index()

if len(metadata) == 0:
    print(
        "Warning [vfc_ci]: No run files matched the specified timeframe. "
        "This will result in server errors and prevent you from viewing the report."
        "If you did not expect this, make sure that you have correctly "
        "specified the directory containing the run files.", file=sys.stderr)


# Sort and filter metadata

metadata.sort_index()

# max_files will equal the actual dataframe size if its smaller than the original
# value
max_files = min(max_files, len(metadata))

metadata = metadata.head(max_files)

# Minimum acceptable timestamp
min_timestamp = metadata.iloc[max_files - 1].name

# Second run for data (now that we know which files to load entirely)
for f in run_files:

    # We have to read the metadata again to get back the timestamp. If
    # it is most recent than min_timestamp, the data is loaded.
    path = os.path.normpath(directory + "/" + f)
    current_metadata = pd.read_hdf(path, "metadata")
    current_timestamp = current_metadata.iloc[0].name

    if current_timestamp <= min_timestamp:
        data.append(pd.read_hdf(directory + "/" + f, "data"))

data = pd.concat(data).sort_index()

# Generate the display strings for runs (runs ticks)
# By doing this in master, we ensure the homogeneity of display strings
# across all plots
metadata["name"] = metadata.index.to_series().map(
    lambda x: helper.get_run_name(
        x,
        helper.get_metadata(metadata, x)["hash"]
    )
)
helper.reset_run_strings()

metadata["date"] = metadata.index.to_series().map(
    lambda x: time.ctime(x)
)


##########################################################################

# Setup report views


class ViewsMaster:
    '''
    The ViewsMaster class allows two-ways communication between views.
    This approach by classes allows us to have separate scopes for each view
    and will be useful if we want to add new views at some point in the future
    (instead of having n views with n-1 references each).
    '''

    # Callbacks

    def change_repo(self, attrname, old, new):
        filtered_data = self.data[
            helper.filterby_repo(
                self.metadata, new, self.data["timestamp"]
            )
        ]
        filtered_metadata = self.metadata[
            self.metadata["repo_name"] == new
        ]

        self.compare.change_repo(filtered_data, filtered_metadata)
        self.inspect.change_repo(filtered_data, filtered_metadata)

        # Communication functions

    def go_to_inspect(self, run_name):
        self.inspect.switch_view(run_name)

        # Constructor

    def __init__(self, data, metadata):

        curdoc().title = "Verificarlo Report"

        self.data = data
        self.metadata = metadata

        # Initialize repository selection

        # Generate display names for repositories
        remote_urls = self.metadata["remote_url"].drop_duplicates().to_list()
        branches = self.metadata["branch"].drop_duplicates().to_list()
        repo_names_dict = helper.gen_repo_names(remote_urls, branches)
        self.metadata["repo_name"] = self.metadata["remote_url"].apply(
            lambda x: repo_names_dict[x]
        )

        # Add the repository selection widget
        select_repo = Select(
            name="select_repo", title="",
            value=list(repo_names_dict.values())[0],
            options=list(repo_names_dict.values())
        )
        curdoc().add_root(select_repo)

        select_repo.on_change("value", self.change_repo)

        change_repo_callback_js = "changeRepository(cb_obj.value);"
        select_repo.js_on_change(
            "value",
            CustomJS(code=change_repo_callback_js)
        )

        # Invert key/values for repo_names_dict and pass it to the template as
        # a JSON string
        repo_names_dict = {
            value: key for key,
            value in repo_names_dict.items()}
        curdoc().template_variables["repo_names_dict"] = json.dumps(
            repo_names_dict)

        # Pass metadata to the template as a JSON string
        curdoc().template_variables["metadata"] = self.metadata.to_json(
            orient="index")

        repo_name = list(repo_names_dict.keys())[0]

        # Filter data and metadata by repository
        filtered_data = self.data[
            helper.filterby_repo(
                self.metadata, repo_name, self.data["timestamp"]
            )
        ]
        filtered_metadata = self.metadata[
            self.metadata["repo_name"] == repo_name
        ]

        # Initialize views

        # Initialize runs comparison
        self.compare = compare_runs.CompareRuns(
            master=self,
            doc=curdoc(),
            data=filtered_data,
            metadata=filtered_metadata,
        )

        # Initialize runs inspection
        self.inspect = inspect_runs.InspectRuns(
            master=self,
            doc=curdoc(),
            data=filtered_data,
            metadata=filtered_metadata,
        )


views_master = ViewsMaster(
    data=data,
    metadata=metadata
)
