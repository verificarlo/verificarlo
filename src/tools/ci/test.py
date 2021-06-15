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

# This script reads the vfc_tests_config.json file and executes tests accordingly
# It will also generate a ... .vfcrunh5 file with the results of the run

from .test_data_processing import data_processing
import pandas as pd
import os
import subprocess
import sys
import json
import tempfile
import calendar
import time

# Forcing an older pickle protocol allows backwards compatibility when reading
# HDF5 written in 3.8+ with an older version of Python
import pickle
pickle.HIGHEST_PROTOCOL = 4


# Magic numbers
timeout = 600   # For commands execution


##########################################################################

# Helper functions

def read_probes_csv(filepath, backend, warnings, execution_data):
    '''Read a CSV file outputted by vfc_probe as a Pandas dataframe'''

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
            "them without calling vfc_probe"
        )
        warnings.append(execution_data)

    # Once the CSV has been opened and validated, return its content
    results["value"] = results["value"].apply(lambda x: float.fromhex(x))
    results.rename(columns={"value": "values"}, inplace=True)

    results["vfc_backend"] = backend

    return results


##########################################################################

    # Main functions


def read_config():
    '''Open and read the tests config file'''

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


def run_tests(config):
    '''
    Execute tests and collect results in a Pandas dataframe
    '''

    # Run the build command
    print("Info [vfc_ci]: Building tests...")
    os.system(config["make_command"])

    # This is an array of Pandas dataframes for now
    data = []

    # This will contain all executables/repetition numbers from which we could
    # not get any data
    warnings = []

    # Tests execution loops

    for executable in config["executables"]:
        print("Info [vfc_ci]: Running executable :",
              executable["executable"], "...")

        parameters = ""
        if "parameters" in executable:
            parameters = executable["parameters"]

        for backend in executable["vfc_backends"]:

            os.putenv("VFC_BACKENDS", backend["name"])

            command = "./" + executable["executable"] + " " + parameters

            repetitions = 1
            if "repetitions" in backend:
                repetitions = backend["repetitions"]

            # Run test repetitions and save results
            for i in range(repetitions):
                temp = tempfile.NamedTemporaryFile()
                os.putenv("VFC_PROBES_OUTPUT", temp.name)

                print(command)
                p = subprocess.Popen(command.split())
                try:
                    p.wait(timeout)
                except subprocess.TimeoutExpired:
                    print(
                        "Warning [vfc_ci]: execution was timed out",
                        file=sys.stderr)
                    p.kill()

                # This will only be used if we need to append this exec to the
                # warnings list
                execution_data = {
                    "executable": executable["executable"],
                    "backend": backend["name"],
                    "repetition": i + 1
                }

                data.append(read_probes_csv(
                    temp.name,
                    backend["name"],
                    warnings,
                    execution_data
                ))

                temp.close()

    # Combine all separate executions in one dataframe
    data = pd.concat(data, sort=False, ignore_index=True)
    data = data.groupby(["test", "vfc_backend", "variable"])\
        .values.apply(list).reset_index()

    # Make sure we have some data to work on
    assert(len(data) != 0), "Error [vfc_ci]: No data have been generated " \
        "by your tests executions, aborting run without writing results file"

    return data, warnings


def show_warnings(warnings):
    '''
    Display all executions that resulted in a warning
    '''

    if len(warnings) > 0:
        print(
            "Warning [vfc_ci]: Some of your runs could not generate any data "
            "(for instance because your code crashed) and resulted in "
            "warnings. Here is the complete list :",
            file=sys.stderr
        )

        for i in range(0, len(warnings)):
            print("- Warning %s:" % i, file=sys.stderr)

            print(
                "  Executable: %s" %
                warnings[i]["executable"],
                file=sys.stderr)
            print("  Backend: %s" % warnings[i]["backend"], file=sys.stderr)
            print(
                "  Repetition: %s" %
                warnings[i]["repetition"],
                file=sys.stderr)


##########################################################################

def run(is_git_commit, export_raw_values, dry_run):
    '''Entry point of vfc_ci test'''

    # Get config, metadata and data
    print("Info [vfc_ci]: Reading tests config file...")
    config = read_config()

    print("Info [vfc_ci]: Generating run metadata...")
    metadata = generate_metadata(is_git_commit)

    data, warnings = run_tests(config)
    show_warnings(warnings)

    # Data processing
    data = data_processing(data)

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
