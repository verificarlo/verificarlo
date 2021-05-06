# General helper functions for both compare_runs and compare_variables

import calendar
import time
from itertools import compress

import numpy as np

# Magic numbers
max_ticks = 15
max_zscore = 3

##########################################################################


# From a timestamp, return the associated metadata as a Pandas serie
def get_metadata(metadata, timestamp):
    return metadata.loc[timestamp]


# Convert a metadata Pandas series to a JS readable dict
def metadata_to_dict(metadata):
    dict = metadata.to_dict()

    # JS doesn't accept True for booleans, and Python doesn't accept true
    # (because of the caps) => using an integer is a portable solution
    dict["is_git_commit"] = 1 if dict["is_git_commit"] else 0

    dict["date"] = time.ctime(metadata.name)

    return dict


# Return a string that indicates the elapsed time since the run, used as the
# x-axis tick in "Compare runs" or when selecting run in "Inspect run"
def get_run_name(timestamp, hash):

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
        n = diff / 2592000
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
# avoid duplicates (assuming the runs are sorted by time)
get_run_name.counter = 0
get_run_name.previous = ""


def reset_run_strings():
    get_run_name.counter = 0
    get_run_name.previous = ""


# Update all the x-ranges from a dict of plots
def reset_x_range(plot, x_range):
    plot.x_range.factors = x_range

    if len(x_range) < max_ticks:
        plot.xaxis.major_tick_line_color = "#000000"
        plot.xaxis.minor_tick_line_color = "#000000"

        plot.xaxis.major_label_text_font_size = "8pt"

    else:
        plot.xaxis.major_tick_line_color = None
        plot.xaxis.minor_tick_line_color = None

        plot.xaxis.major_label_text_font_size = "0pt"


# Return an array of booleans that indicate which elements are outliers
# (True means element is not an outlier and must be kept)
def detect_outliers(array, max_zscore=max_zscore):
    if len(array) <= 2:
        return [True] * len(array)

    median = np.median(array)
    std = np.std(array)
    if std == 0:
        return array
    distance = abs(array - median)
    # Array of booleans with elements to be filtered
    outliers_array = distance < max_zscore * std

    return outliers_array


def remove_outliers(array, outliers):
    return list(compress(array, outliers))


def remove_boxplot_outliers(dict, outliers, prefix):
    outliers = detect_outliers(dict["%s_max" % prefix])

    dict["%s_x" % prefix] = remove_outliers(dict["%s_x" % prefix], outliers)

    dict["%s_min" % prefix] = remove_outliers(
        dict["%s_min" % prefix], outliers)
    dict["%s_quantile25" % prefix] = remove_outliers(
        dict["%s_quantile25" % prefix], outliers)
    dict["%s_quantile50" % prefix] = remove_outliers(
        dict["%s_quantile50" % prefix], outliers)
    dict["%s_quantile75" % prefix] = remove_outliers(
        dict["%s_quantile75" % prefix], outliers)
    dict["%s_max" % prefix] = remove_outliers(
        dict["%s_max" % prefix], outliers)
    dict["%s_mu" % prefix] = remove_outliers(dict["%s_mu" % prefix], outliers)

    dict["nsamples"] = remove_outliers(dict["nsamples"], outliers)
