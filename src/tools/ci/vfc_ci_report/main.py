# Look for and read all the run files in the current directory (ending with
# .vfcrunh5), and lanch a Bokeh server for the visualization of this data.

import os
import sys
import time

import pandas as pd

from bokeh.plotting import curdoc

# Local imports from vfc_ci_server
import compare_runs
import inspect_runs
import helper

##########################################################################

# Read vfcrun files, and aggregate them in one dataset

run_files = [f for f in os.listdir(".") if f.endswith(".vfcrun.h5")]

if len(run_files) == 0:
    print(
        "Warning [vfc_ci]: Could not find any vfcrun files in the directory. "
        "This will result in server errors and prevent you from viewing the report.")

# These are arrays of Pandas dataframes for now
metadata = []
data = []

for f in run_files:
    metadata.append(pd.read_hdf(f, "metadata"))
    data.append(pd.read_hdf(f, "data"))

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


curdoc().title = "Verificarlo Report"

# Read server arguments
# (this is quite easy because Bokeh server is called through a wrapper, so
# we know exactly what the arguments might be)

git_repo_linked = False
commit_link = ""

has_logo = False
logo_url = ""

for i in range(1, len(sys.argv)):

    # Look for the Git repository remote address
    # (if a Git repo is specified, the webpage will contain hyperlinks to the
    # repository and the different commits)
    if sys.argv[i] == "git":
        from urllib.parse import urlparse

        method = sys.argv[i + 1]
        address = sys.argv[i + 2]
        url = ""

        # Here, address is either the remote URL or the path to the local Git
        # repo (depending on the method)

        if method == "url":
            # We should directly have a Git URL
            url = address

        elif method == "directory":
            # Get the remote URL from the local repo
            from git import Repo
            repo = Repo(address)
            url = repo.remotes.origin.url

        else:
            raise ValueError(
                "Error [vfc_ci]: The specified method to get the Git "
                "repository is invalid. Are you calling Bokeh directly "
                "instead of using the Verificarlo wrapper ?"
            )

        # At this point, "url" should be set correctly, we can get the repo's
        # URL and name, after making sure we're on a Git URL

        parsed_url = urlparse(url)

        path = parsed_url.path.split("/")
        if len(path) < 3:
            raise ValueError(
                "Error [vfc_ci]: The found URL doesn't seem to be pointing "
                "to a Git repository (path is too short)"
            )

        repo_name = path[2]

        curdoc().template_variables["repo_url"] = url
        curdoc().template_variables["repo_name"] = repo_name

        # We should have a "github.com" or a "*gitlab*" URL

        if parsed_url.netloc == "github.com":
            commit_link = "https://%s%s/commit/" \
                % (parsed_url.netloc, parsed_url.path)

            curdoc().template_variables["commit_link"] = commit_link
            curdoc().template_variables["git_host"] = "GitHub"

            # Used in Bokeh tooltips
            commit_link = commit_link + "@hash"

        # We assume we have a GitLab URL
        else:
            commit_link = "https://%s%s/-/commit/" \
                % (parsed_url.netloc, parsed_url.path)

            curdoc().template_variables["commit_link"] = commit_link
            curdoc().template_variables["git_host"] = "GitLab"

            # Used in Bokeh tooltips
            commit_link = commit_link + "@hash"

        git_repo_linked = True

    # Look for a logo URL
    # If a logo URL is specified, it will be included in the report's header
    if sys.argv[i] == "logo":
        curdoc().template_variables["logo_url"] = sys.argv[i + 1]
        has_logo = True


# After the loop, we know if a repo has been linked, if we have a logo, ...
curdoc().template_variables["git_repo_linked"] = git_repo_linked
curdoc().template_variables["has_logo"] = has_logo


##########################################################################

# Setup report views

# Define a ViewsMaster class to allow two-ways communication between views.
# This approach by classes allows us to have separate scopes for each view and
# will be useful if we want to add new views at some point in the future
# (instead of having n views with n-1 references each).

class ViewsMaster:

    # Communication functions

    def go_to_inspect(self, run_name):
        self.inspect.switch_view(run_name)

        # Constructor

    def __init__(self, data, metadata, git_repo_linked, commit_link):

        self.data = data
        self.metadata = metadata
        self.git_repo_linked = git_repo_linked
        self.commit_link = commit_link

        # Pass metadata to the template as a JSON string
        curdoc().template_variables["metadata"] = self.metadata.to_json(
            orient="index")

        # Runs comparison
        self.compare = compare_runs.CompareRuns(
            master=self,
            doc=curdoc(),
            data=data,
            metadata=metadata,
        )

        # Runs inspection
        self.inspect = inspect_runs.InspectRuns(
            master=self,
            doc=curdoc(),
            data=data,
            metadata=metadata,
        )


views_master = ViewsMaster(
    data=data,
    metadata=metadata,
    git_repo_linked=git_repo_linked,
    commit_link=commit_link
)
