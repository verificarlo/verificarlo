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

# General helper functions for both compare_runs and compare_variables

import calendar
import time
from itertools import compress
from urllib.parse import urlparse

import numpy as np

# Magic numbers
max_ticks = 15
max_zscore = 3

##########################################################################


def gen_repo_names(remote_urls, branches):
    """
    Generate display repository names from (sorted and equally sized) lists of
    remote URLs and branch names
    """

    repo_names_dict = {}

    for i in range(0, len(remote_urls)):
        if remote_urls[i] == "":
            repo_names_dict[remote_urls[i]] = "None"
            continue

        parsed_url = urlparse(remote_urls[i])
        path = parsed_url.path.split("/")

        repo_name = path[-2] + "/" + path[-1] + ":" + branches[i]
        repo_names_dict[remote_urls[i]] = repo_name

    return repo_names_dict


def get_metadata(metadata, timestamp):
    """From a timestamp, return the associated metadata as a Pandas serie"""

    return metadata.loc[timestamp]


def gen_x_series(metadata, timestamps, current_n_runs):
    """
    From an array of timestamps, returns the array of runs names (for the x
    axis ticks), as well as the metadata (in a dict of arrays) associated
    to this array (will be used in tooltips)
    """

    # Initialize the objects to return
    x_series = []
    x_metadata = dict(date=[], is_git_commit=[], hash=[], author=[], message=[])

    # n == 0 means we want all runs, we also make sure not to go out of
    # bound if asked for more runs than we have
    n = current_n_runs
    if n == 0 or n > len(timestamps):
        n = len(timestamps)

    for i in range(0, n):
        # Get metadata associated to this run
        row_metadata = get_metadata(metadata, timestamps[-i - 1])
        date = time.ctime(timestamps[-i - 1])

        # Fill the x series
        str = row_metadata["name"]
        x_series.insert(0, get_metadata(metadata, timestamps[-i - 1])["name"])

        # Fill the metadata lists
        x_metadata["date"].insert(0, date)
        x_metadata["is_git_commit"].insert(0, row_metadata["is_git_commit"])
        x_metadata["hash"].insert(0, row_metadata["hash"])
        x_metadata["author"].insert(0, row_metadata["author"])
        x_metadata["message"].insert(0, row_metadata["message"])

    return x_series, x_metadata


def filterby_repo(metadata, repo_name, timestamps):
    """
    Returns a boolean to indicate if a timestamp (rather the commit linked to it)
    belongs to a particular repository
    """

    return timestamps.apply(lambda x: get_metadata(metadata, x) == repo_name)[
        "repo_name"
    ]


def metadata_to_dict(metadata):
    """
    Convert a metadata Pandas series to a JS readable dict
    """

    dict = metadata.to_dict()

    # JS doesn't accept True for booleans, and Python doesn't accept true
    # (because of the caps) => using an integer is a portable solution
    dict["is_git_commit"] = 1 if dict["is_git_commit"] else 0

    dict["date"] = time.ctime(metadata.name)

    return dict


def get_run_name(timestamp, hash):
    """
    Return a string that indicates the elapsed time since the run, used as the
    x-axis tick in "Compare runs" or when selecting run in "Inspect run"
    """

    gmt = time.gmtime()
    now = calendar.timegm(gmt)
    diff = now - timestamp

    # Special case : < 1 minute (return string directly)
    if diff < 60:
        str = "Less than a minute ago"

        if hash != "":
            str = str + " (%s)" % hash

        if str == get_run_name.previous:
            get_run_name.counter = get_run_name.counter + 1
            str = "%s (%s)" % (str, get_run_name.counter)
        else:
            get_run_name.counter = 0
            get_run_name.previous = str

        return str

    # < 1 hour
    if diff < 3600:
        n = int(diff / 60)
        str = "%s minute%s ago"
    # < 1 day
    elif diff < 86400:
        n = int(diff / 3600)
        str = "%s hour%s ago"
    # < 1 week
    elif diff < 604800:
        n = int(diff / 86400)
        str = "%s day%s ago"
    # < 1 month
    elif diff < 2592000:
        n = int(diff / 604800)
        str = "%s week%s ago"
    # > 1 month
    else:
        n = int(diff / 2592000)
        str = "%s month%s ago"

    plural = ""
    if n != 1:
        plural = "s"

    str = str % (n, plural)

    # We might want to add the git hash
    if hash != "":
        str = str + " (%s)" % hash

    # Finally, check for duplicate with previously generated string
    if str == get_run_name.previous:
        # Increment the duplicate counter and add it to str
        get_run_name.counter = get_run_name.counter + 1
        str = "%s (%s)" % (str, get_run_name.counter)

    else:
        # No duplicate, reset both previously generated str and duplicate
        # counter
        get_run_name.counter = 0
        get_run_name.previous = str

    return str


# These external variables will store data about the last generated string to
# avoid duplicates (assuming the runs are sorted chronologically)
get_run_name.counter = 0
get_run_name.previous = ""


def reset_run_strings():
    get_run_name.counter = 0
    get_run_name.previous = ""


def reset_x_range(plot, x_range):
    """
    Update all the x-ranges from a dict of plots
    """

    plot.x_range.factors = x_range

    if len(x_range) < max_ticks:
        plot.xaxis.major_tick_line_color = "#000000"
        plot.xaxis.minor_tick_line_color = "#000000"

        plot.xaxis.major_label_text_font_size = "8pt"

    else:
        plot.xaxis.major_tick_line_color = None
        plot.xaxis.minor_tick_line_color = None

        plot.xaxis.major_label_text_font_size = "0pt"


def detect_outliers(array, max_zscore=max_zscore):
    """
    Return an array of booleans that indicate which elements are outliers
    (True means element is not an outlier and must be kept)
    """

    if len(array) <= 2:
        return [True] * len(array)

    mean = np.mean(array)
    std = np.std(array)
    if std == 0:
        return array
    distance = abs(array - mean)
    # Array of booleans with elements to be filtered
    outliers_array = distance < max_zscore * std

    return outliers_array


def remove_outliers(array, outliers):
    return list(compress(array, outliers))


def remove_boxplot_outliers(dict, outliers, prefix):
    outliers = detect_outliers(dict["%s_max" % prefix])

    dict["%s_x" % prefix] = remove_outliers(dict["%s_x" % prefix], outliers)

    dict["%s_min" % prefix] = remove_outliers(dict["%s_min" % prefix], outliers)
    dict["%s_quantile25" % prefix] = remove_outliers(
        dict["%s_quantile25" % prefix], outliers
    )
    dict["%s_quantile50" % prefix] = remove_outliers(
        dict["%s_quantile50" % prefix], outliers
    )
    dict["%s_quantile75" % prefix] = remove_outliers(
        dict["%s_quantile75" % prefix], outliers
    )
    dict["%s_max" % prefix] = remove_outliers(dict["%s_max" % prefix], outliers)
    dict["%s_mu" % prefix] = remove_outliers(dict["%s_mu" % prefix], outliers)

    dict["nsamples"] = remove_outliers(dict["nsamples"], outliers)


def gen_runs_selection(metadata):
    """
    Returns a dictionary mapping user-readable strings to all run timestamps
    """

    runs_dict = {}

    # Iterate over timestamp rows (runs) and fill dict
    for row in metadata.iloc:
        # The syntax used by pandas makes this part a bit tricky :
        # row.name is the index of metadata (so it refers to the
        # timestamp), whereas row["name"] is the column called "name"
        # (which is the display string used for the run)

        # runs_dict[run's name] = run's timestamp
        runs_dict[row["name"]] = row.name

    return runs_dict
