# This script reads the vfc_tests_config.json file and executes tests accordingly
# It will also generate a ... .vfcrunh5 file with the results of the run

import verificarlo.sigdigits as sd
import scipy.stats
import numpy as np
import pandas as pd
import os
import json

import calendar
import time

# Forcing an older pickle protocol allows backwards compatibility when reading
# HDF5 written in 3.8+ with an older version of Python
import pickle
pickle.HIGHEST_PROTOCOL = 4


# Magic numbers
min_pvalue = 0.05
max_zscore = 3


##########################################################################

# Helper functions

# Read a CSV file outputted by vfc_probe as a Pandas dataframe
def read_probes_csv(filepath, backend, warnings, execution_data):

    try:
        results = pd.read_csv(filepath)

    except FileNotFoundError:
        print(
            "Warning [vfc_ci]: Probes not found, your code might have crashed "
            "or you might have forgotten to call vfc_dump_probes"
        )
        warnings.append(execution_data)
        return pd.DataFrame(
            columns=["test", "variable", "values", "vfc_backend"]
        )

    except Exception:
        print(
            "Warning [vfc_ci]: Your probes could not be read for some unknown "
            "reason"
        )
        warnings.append(execution_data)
        return pd.DataFrame(
            columns=["test", "variable", "values", "vfc_backend"]
        )

    if len(results) == 0:
        print(
            "Warning [vfc_ci]: Probes empty, it looks like you have dumped "
            "them without calling vfc_put_probe"
        )
        warnings.append(execution_data)

    # Once the CSV has been opened and validated, return its content
    results["value"] = results["value"].apply(lambda x: float.fromhex(x))
    results.rename(columns={"value": "values"}, inplace=True)

    results["vfc_backend"] = backend

    return results


# First wrapper to sd.significant_digits (returns results in base 2)

def significant_digits(x):

    # If the null hypothesis is rejected, call sigdigits with the General
    # formula:
    if x.pvalue < min_pvalue:
        # In a pandas DF, "values" actually refers to the array of columns, and
        # not the column named "values"
        distribution = x.values[3]
        distribution = distribution.reshape(len(distribution), 1)

        # The distribution's empirical average will be used as the reference
        mu = np.array([x.mu])

        s = sd.significant_digits(
            distribution,
            mu,
            precision=sd.Precision.Relative,
            method=sd.Method.General,

            probability=0.9,
            confidence=0.95
        )

        # s is returned inside a list
        return s[0]

    # Else, manually compute sMCA (Stott-Parker formula)
    else:
        return -np.log2(np.absolute(x.sigma / x.mu))


# First wrapper to sd.significant_digits : assumes s2 has already been computed

def significant_digits_lower_bound(x):
    # If the null hypothesis is rejected, no lower bound
    if x.pvalue < min_pvalue:
        return x.s2

    # Else, the lower bound will be computed with p= .9 alpha-1=.95
    else:
        distribution = x.values[3]
        distribution = distribution.reshape(len(distribution), 1)

        mu = np.array([x.mu])

        s = sd.significant_digits(
            distribution,
            mu,
            precision=sd.Precision.Relative,
            method=sd.Method.CNH,

            probability=0.9,
            confidence=0.95
        )

        return s[0]


##########################################################################

    # Main functions


# Open and read the tests config file
def read_config():
    try:
        with open("vfc_tests_config.json", "r") as file:
            data = file.read()

    except FileNotFoundError as e:
        e.strerror = "Error [vfc_ci]: This file is required to describe the tests "\
            "to run and generate a Verificarlo run file"
        raise e

    return json.loads(data)


# Set up metadata
def generate_metadata(is_git_commit):

    # Metadata and filename are initiated as if no commit was associated
    metadata = {
        "timestamp": calendar.timegm(time.gmtime()),
        "is_git_commit": is_git_commit,
        "remote_url": "",
        "branch": "",
        "hash": "",
        "author": "",
        "message": ""
    }

    if is_git_commit:
        print("Fetching metadata from last commit...")
        from git import Repo

        repo = Repo(".")

        remote_url = repo.remotes[0].config_reader.get("url")
        metadata["remote_url"] = remote_url

        branch = repo.active_branch.name
        metadata["branch"] = branch

        head_commit = repo.head.commit

        metadata["timestamp"] = head_commit.authored_date

        metadata["hash"] = str(head_commit)[0:7]
        metadata["author"] = "%s <%s>" \
            % (str(head_commit.author), head_commit.author.email)
        metadata["message"] = head_commit.message.split("\n")[0]

    return metadata


# Execute tests and collect results in a Pandas dataframe (+ dataprocessing)
def run_tests(config):

    # Run the build command
    print("Info [vfc_ci]: Building tests...")
    os.system(config["make_command"])

    # This is an array of Pandas dataframes for now
    data = []

    # Create tmp folder to export results
    os.system("mkdir .vfcruns.tmp")
    n_files = 0

    # This will contain all executables/repetition numbers from which we could
    # not get any data
    warnings = []

    # Tests execution loop
    for executable in config["executables"]:
        print("Info [vfc_ci]: Running executable :",
              executable["executable"], "...")

        parameters = ""
        if "parameters" in executable:
            parameters = executable["parameters"]

        for backend in executable["vfc_backends"]:

            export_backend = "VFC_BACKENDS=\"" + backend["name"] + "\" "
            command = "./" + executable["executable"] + " " + parameters

            repetitions = 1
            if "repetitions" in backend:
                repetitions = backend["repetitions"]

            # Run test repetitions and save results
            for i in range(repetitions):
                file = ".vfcruns.tmp/%s.csv" % str(n_files)
                export_output = "VFC_PROBES_OUTPUT=\"%s\" " % file
                os.system(export_output + export_backend + command)

                # This will only be used if we need to append this exec to the
                # warnings list
                execution_data = {
                    "executable": executable["executable"],
                    "backend": backend["name"],
                    "repetition": i + 1
                }

                data.append(read_probes_csv(
                    file,
                    backend["name"],
                    warnings,
                    execution_data
                ))

                n_files = n_files + 1

    # Clean CSV output files (by deleting the tmp folder)
    os.system("rm -rf .vfcruns.tmp")

    # Combine all separate executions in one dataframe
    data = pd.concat(data, sort=False, ignore_index=True)
    data = data.groupby(["test", "vfc_backend", "variable"])\
        .values.apply(list).reset_index()

    # Make sure we have some data to work on
    assert(len(data) != 0), "Error [vfc_ci]: No data have been generated " \
        "by your tests executions, aborting run without writing results file"

    return data, warnings

    # Data processing : computes all metrics on the dataframe


def data_processing(data):

    data["values"] = data["values"].apply(lambda x: np.array(x).astype(float))

    # Get empirical average, standard deviation and p-value
    data["mu"] = data["values"].apply(np.average)
    data["sigma"] = data["values"].apply(np.std)
    data["pvalue"] = data["values"].apply(
        lambda x: scipy.stats.shapiro(x).pvalue)

    # Significant digits
    data["s2"] = data.apply(significant_digits, axis=1)
    data["s10"] = data["s2"].apply(lambda x: sd.change_base(x, 10))

    # Lower bound of the confidence interval using the sigdigits module
    data["s2_lower_bound"] = data.apply(significant_digits_lower_bound, axis=1)
    data["s10_lower_bound"] = data["s2_lower_bound"].apply(
        lambda x: sd.change_base(x, 10))

    # Compute moments of the distribution
    # (including a new distribution obtained by filtering outliers)
    data["values"] = data["values"].apply(np.sort)

    data["mu"] = data["values"].apply(np.average)
    data["min"] = data["values"].apply(np.min)
    data["quantile25"] = data["values"].apply(np.quantile, args=(0.25,))
    data["quantile50"] = data["values"].apply(np.quantile, args=(0.50,))
    data["quantile75"] = data["values"].apply(np.quantile, args=(0.75,))
    data["max"] = data["values"].apply(np.max)

    data["nsamples"] = data["values"].apply(len)

    # Display all executions that resulted in a warning


def show_warnings(warnings):
    if len(warnings) > 0:
        print(
            "Warning [vfc_ci]: Some of your runs could not generate any data "
            "(for instance because your code crashed) and resulted in "
            "warnings. Here is the complete list :"
        )

        for i in range(0, len(warnings)):
            print("- Warning %s:" % i)

            print("  Executable: %s" % warnings[i]["executable"])
            print("  Backend: %s" % warnings[i]["backend"])
            print("  Repetition: %s" % warnings[i]["repetition"])


##########################################################################

# Entry point of vfc_ci test
def run(is_git_commit, export_raw_values, dry_run):

    # Get config, metadata and data
    print("Info [vfc_ci]: Reading tests config file...")
    config = read_config()

    print("Info [vfc_ci]: Generating run metadata...")
    metadata = generate_metadata(is_git_commit)

    data, warnings = run_tests(config)
    show_warnings(warnings)

    # Data processing
    print("Info [vfc_ci]: Processing data...")
    data_processing(data)

    # Prepare data for export (by creating a proper index and linking run
    # timestamp)
    data = data.set_index(["test", "variable", "vfc_backend"]).sort_index()
    data["timestamp"] = metadata["timestamp"]

    filename = metadata["hash"] if is_git_commit else str(
        metadata["timestamp"])

    # Prepare metadata for export
    metadata = pd.DataFrame.from_dict([metadata])
    metadata = metadata.set_index("timestamp")

    # WARNING : Exporting to HDF5 implicitly requires to install "tables" on the
    # system

    # Export raw data if needed
    if export_raw_values and not dry_run:
        data.to_hdf(filename + ".vfcraw.h5", key="data")
        metadata.to_hdf(filename + ".vfcraw.h5", key="metadata")

    # Export data
    del data["values"]
    if not dry_run:
        data.to_hdf(filename + ".vfcrun.h5", key="data")
        metadata.to_hdf(filename + ".vfcrun.h5", key="metadata")

    # Print termination messages
    print(
        "Info [vfc_ci]: The results have been successfully written to "
        "%s.vfcrun.h5."
        % filename
    )

    if export_raw_values:
        print(
            "Info [vfc_ci]: A file containing the raw values has also been "
            "created : %s.vfcraw.h5."
            % filename
        )

    if dry_run:
        print(
            "Info [vfc_ci]: The dry run flag was enabled, so no files were "
            "actually created."
        )
