# Look for and read all the run files in the current directory (ending with
# .vfcrunh5), and lanch a Bokeh server for the visualization of this data.

import os
import sys
import time
import json

import pandas as pd

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

directory = "."

has_logo = False
logo_url = ""


for i in range(1, len(sys.argv)):

    # Look for a logo URL
    # If a logo URL is specified, it will be included in the report's header
    if sys.argv[i] == "logo":
        curdoc().template_variables["logo_url"] = sys.argv[i + 1]
        has_logo = True

    if sys.argv[i] == "directory":
        directory = sys.argv[i + 1]


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

for f in run_files:
    metadata.append(pd.read_hdf(directory + "/" + f, "metadata"))
    data.append(pd.read_hdf(directory + "/" + f, "data"))

metadata = pd.concat(metadata).sort_index()
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

# Define a ViewsMaster class to allow two-ways communication between views.
# This approach by classes allows us to have separate scopes for each view and
# will be useful if we want to add new views at some point in the future
# (instead of having n views with n-1 references each).

class ViewsMaster:

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
