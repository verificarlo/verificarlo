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

# Look for and read all the run files in the current directory (ending with
# .vfcrun.h5), and lanch a Bokeh server for the visualization of this data.

import os
import sys
import time
import json

import pandas as pd

from bokeh.plotting import curdoc
from bokeh.models import Select, CustomJS

# Local imports from vfc_ci_server
import compare_runs
import deterministic_compare
import inspect_runs
import checks

import helper

##########################################################################


# Read server arguments
# (this is quite easy because Bokeh server is called through a wrapper, so
# we know exactly what the arguments might be)


has_logo = False
logo_url = ""

directory = "."

max_files = 100
ignore_recent = 0

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

    # By default, the n latest files are selected, but this can be modified
    if sys.argv[i] == "ignore_recent":
        ignore_recent = int(sys.argv[i + 1])


curdoc().template_variables["has_logo"] = has_logo


##########################################################################


# Read vfcrun files, and aggregate them in one dataset

run_files = [f for f in os.listdir(directory) if f.endswith(".vfcrun.h5")]

if len(run_files) == 0:
    print(
        "Warning [vfc_ci]: Could not find any vfcrun files in the directory. "
        "This will result in server errors and prevent you from viewing the report."
    )

# These are arrays of Pandas dataframes for now
metadata = []
data = []
deterministic_data = []

# First pass for metadata
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
        "specified the directory containing the run files.",
        file=sys.stderr,
    )


# Sort and filter metadata

metadata.sort_index()

# Ignore the most recent files if needed
if ignore_recent != 0:
    metadata.drop(metadata.tail(ignore_recent).index, inplace=True)

# max_files will equal the actual dataframe size if its smaller than the
# original value
max_files = min(max_files, len(metadata))

metadata = metadata.head(max_files)

# Maximal acceptable timestamp
max_timestamp = metadata.iloc[max_files - 1].name

# Second pass for data (now that we know which files to load entirely)
for f in run_files:
    # We have to read the metadata again to get back the timestamp.
    # Then, depending of the timestamp value, data will be read or dismissed
    path = os.path.normpath(directory + "/" + f)
    current_metadata = pd.read_hdf(path, "metadata")
    current_timestamp = current_metadata.iloc[0].name

    if current_timestamp <= max_timestamp:
        data.append(pd.read_hdf(directory + "/" + f, "data"))
        deterministic_data.append(
            pd.read_hdf(directory + "/" + f, "deterministic_data")
        )

data = pd.concat(data).sort_index()
deterministic_data = pd.concat(deterministic_data).sort_index()

# If no data/deterministic_data has been found, create an empty dataframe anyway
# (with column names) to avoid errors further in the code
if data.empty:
    data = pd.DataFrame(
        columns=[
            "test",
            "variable",
            "backend",
            "sigma",
            "s10",
            "s2",
            "s10_lower_bound",
            "s2_lower_bound",
            "mu",
            "quantile25",
            "quantile50",
            "quantile75",
            "accuracy_threshold",
            "check",
            "check_mode",
            "timestamp",
        ]
    )

if deterministic_data.empty:
    deterministic_data = pd.DataFrame(
        columns=[
            "test",
            "variable",
            "backend",
            "value",
            "accuracy_threshold",
            "reference_value",
            "check",
            "check_mode",
            "timestamp",
        ]
    )

# Generate the display strings for runs (runs ticks)
# By doing this in master, we ensure the homogeneity of display strings
# across all plots
metadata["name"] = metadata.index.to_series().map(
    lambda x: helper.get_run_name(x, helper.get_metadata(metadata, x)["hash"])
)
helper.reset_run_strings()

metadata["date"] = metadata.index.to_series().map(lambda x: time.ctime(x))

##########################################################################

# Setup report views


class ViewsMaster:
    """
    The ViewsMaster class allows two-ways communication between views.
    This approach by classes allows us to have separate scopes for each view
    and will be useful if we want to add new views at some point in the future
    (instead of having n views with n-1 references each).
    """

    # Callbacks

    def change_repo(self, attrname, old, new):
        # Filter metadata by repository
        filtered_metadata = self.metadata[self.metadata["repo_name"] == new]

        # Filter data and deterministic_data by repository
        if not data.empty:
            filtered_data = self.data[
                helper.filterby_repo(self.metadata, new, self.data["timestamp"])
            ]
        else:
            filtered_data = data

        if not deterministic_data.empty:
            filtered_deterministic_data = self.deterministic_data[
                helper.filterby_repo(
                    self.metadata, new, self.deterministic_data["timestamp"]
                )
            ]
        else:
            filtered_deterministic_data = deterministic_data
            # In some cases, index columns might be missing, so we add them just
            # in case (here, the dataframe is empty anyway)
            filtered_deterministic_data = filtered_deterministic_data.assign(
                test="", variable="", backend=""
            )
            filtered_deterministic_data.set_index(
                ["test", "variable", "backend"], inplace=True
            )

        self.compare.change_repo(filtered_data, filtered_metadata)
        self.deterministic.change_repo(filtered_deterministic_data, filtered_metadata)
        self.inspect.change_repo(filtered_data, filtered_metadata)
        self.checks.change_repo(
            filtered_data, filtered_deterministic_data, filtered_metadata
        )

        # Communication functions

    def go_to_inspect(self, run_name):
        self.inspect.switch_view(run_name)

    def go_to_checks(self, run_name):
        self.checks.switch_view(run_name)

        # Constructor

    def __init__(self, data, deterministic_data, metadata):
        curdoc().title = "Verificarlo Report"

        self.data = data
        self.deterministic_data = deterministic_data
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
            name="select_repo",
            title="",
            value=list(repo_names_dict.values())[0],
            options=list(repo_names_dict.values()),
        )
        curdoc().add_root(select_repo)

        select_repo.on_change("value", self.change_repo)

        change_repo_callback_js = "changeRepository(cb_obj.value);"
        select_repo.js_on_change("value", CustomJS(code=change_repo_callback_js))

        # Invert key/values for repo_names_dict and pass it to the template as
        # a JSON string
        repo_names_dict = {value: key for key, value in repo_names_dict.items()}
        curdoc().template_variables["repo_names_dict"] = json.dumps(repo_names_dict)

        # Pass metadata to the template as a JSON string
        curdoc().template_variables["metadata"] = self.metadata.to_json(orient="index")

        # Show the first repository by default
        repo_name = list(repo_names_dict.keys())[0]

        # Filter metadata by repository
        filtered_metadata = self.metadata[self.metadata["repo_name"] == repo_name]

        # Filter data and deterministic_data by repository
        if not data.empty:
            filtered_data = self.data[
                helper.filterby_repo(self.metadata, repo_name, self.data["timestamp"])
            ]
        else:
            filtered_data = data

        if not deterministic_data.empty:
            filtered_deterministic_data = self.deterministic_data[
                helper.filterby_repo(
                    self.metadata, repo_name, self.deterministic_data["timestamp"]
                )
            ]
        else:
            filtered_deterministic_data = deterministic_data

        # Initialize views

        # Initialize runs comparison
        self.compare = compare_runs.CompareRuns(
            master=self, doc=curdoc(), data=filtered_data, metadata=filtered_metadata
        )

        # Initialize deterministic runs comparison
        self.deterministic = deterministic_compare.DeterministicCompare(
            master=self,
            doc=curdoc(),
            data=filtered_deterministic_data,
            metadata=filtered_metadata,
        )

        # Initialize runs inspection
        self.inspect = inspect_runs.InspectRuns(
            master=self, doc=curdoc(), data=filtered_data, metadata=filtered_metadata
        )

        # Initialize checks table view
        self.checks = checks.Checks(
            master=self,
            doc=curdoc(),
            data=filtered_data,
            deterministic_data=filtered_deterministic_data,
            metadata=filtered_metadata,
        )


views_master = ViewsMaster(
    data=data, deterministic_data=deterministic_data, metadata=metadata
)
