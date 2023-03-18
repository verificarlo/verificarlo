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

# This script reads the vfc_tests_config.json file and executes tests accordingly
# It will also generate a ... .vfcrunh5 file with the results of the run

import calendar
import json
import os

# Forcing an older pickle protocol allows backwards compatibility when reading
# HDF5 written in 3.8+ with an older version of Python
import pickle
import subprocess
import sys
import tempfile
import time

import pandas as pd

from .test_data_processing import data_processing, validate_deterministic_probe

pickle.HIGHEST_PROTOCOL = 4


# Magic numbers
timeout = 600  # For commands execution


##########################################################################

# Helper functions


def read_probes_csv(filepath, warnings, execution_data):
    """Read a CSV file outputted by vfc_probe as a Pandas dataframe"""

    try:
        results = pd.read_csv(filepath)

    except FileNotFoundError:
        print(
            "Warning [vfc_ci]: Probes not found, your code might have crashed "
            "or you might have forgotten to call vfc_dump_probes"
        )
        warnings.append(execution_data)
        results = pd.DataFrame(
            columns=[
                "test",
                "variable",
                "values",
                "accuracy_threshold",
                "check_mode",
                "vfc_backend",
            ]
        )

        checks_data = results[
            ["test", "variable", "vfc_backend", "accuracy_threshold", "check_mode"]
        ].copy()

        return results, checks_data

    except Exception:
        print(
            "Warning [vfc_ci]: Your probes could not be read for some unknown " "reason"
        )
        warnings.append(execution_data)
        results = pd.DataFrame(
            columns=[
                "test",
                "variable",
                "values",
                "accuracy_threshold",
                "check_mode",
                "vfc_backend",
            ]
        )

        checks_data = results[
            ["test", "variable", "vfc_backend", "accuracy_threshold", "check_mode"]
        ].copy()

        return results, checks_data

    if len(results) == 0:
        print(
            "Warning [vfc_ci]: Probes empty, it looks like you have dumped "
            "them without creating any probe"
        )
        warnings.append(execution_data)

    # Once the CSV has been opened and validated, return its content
    results["value"] = results["value"].apply(lambda x: float.fromhex(x))
    results.rename(columns={"value": "values"}, inplace=True)

    results["accuracy_threshold"] = results["accuracy_threshold"].apply(
        lambda x: float.fromhex(x)
    )

    results["vfc_backend"] = execution_data["backend"]

    # Extract accuracy thresholds data
    checks_data = results[
        ["test", "variable", "vfc_backend", "accuracy_threshold", "check_mode"]
    ].copy()

    del results["accuracy_threshold"]
    del results["check_mode"]

    return results, checks_data


##########################################################################

# Main functions


def read_config():
    """Open and read the tests config file"""

    try:
        with open("vfc_tests_config.json", "r") as file:
            data = file.read()

    except FileNotFoundError as e:
        e.strerror = (
            "Error [vfc_ci]: This file is required to describe the tests "
            "to run and generate a Verificarlo run file"
        )
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
        "message": "",
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
        metadata["author"] = "%s <%s>" % (
            str(head_commit.author),
            head_commit.author.email,
        )
        metadata["message"] = head_commit.message.split("\n")[0]

    return metadata


def run_non_deterministic(
    command, repetitions, executable, backend, data, checks_data, warnings
):
    """
    Loop execution for non-deterministic backends. This will also export checks
    so they can be merged later with the data (after the likely duplicates have
    been removed).
    """

    # Run test repetitions and save results
    for i in range(repetitions):
        temp = tempfile.NamedTemporaryFile()
        os.putenv("VFC_PROBES_OUTPUT", temp.name)

        p = subprocess.Popen(command.split())
        try:
            p.wait(timeout)
        except subprocess.TimeoutExpired:
            print("Warning [vfc_ci]: execution was timed out", file=sys.stderr)
            p.kill()

        execution_data = {
            "executable": executable,
            "backend": backend,
            "repetition": i + 1,
        }

        run_data, run_check_data = read_probes_csv(temp.name, warnings, execution_data)

        data.append(run_data)
        checks_data.append(run_check_data)

        temp.close()


def run_deterministic(command, executable, backend, deterministic_data, warnings):
    """
    Single execution for deterministic test. If some probes are associated to an
    check, an IEEE run will also be executed as a reference. The check will be
    checked directly, since it doesn't really require any data processing.
    """

    temp = tempfile.NamedTemporaryFile()
    os.putenv("VFC_PROBES_OUTPUT", temp.name)

    p = subprocess.Popen(command.split())
    try:
        p.wait(timeout)
    except subprocess.TimeoutExpired:
        print("Warning [vfc_ci]: execution was timed out", file=sys.stderr)
        p.kill()

    execution_data = {"executable": executable, "backend": backend, "repetition": 1}

    run_data, run_checks_data = read_probes_csv(temp.name, warnings, execution_data)

    run_data.rename(columns={"values": "value"}, inplace=True)
    run_data["accuracy_threshold"] = run_checks_data["accuracy_threshold"]
    run_data["check_mode"] = run_checks_data["check_mode"]

    # If checks are detected, do a reference run (with IEEE backend)
    if run_data["accuracy_threshold"].sum() != 0:
        temp.close()

        os.putenv("VFC_BACKENDS", "libinterflop_ieee.so")
        temp = tempfile.NamedTemporaryFile()
        os.putenv("VFC_PROBES_OUTPUT", temp.name)

        p = subprocess.Popen(command.split())
        try:
            p.wait(timeout)
        except subprocess.TimeoutExpired:
            print(
                "Warning [vfc_ci]: reference execution was timed out", file=sys.stderr
            )
            p.kill()

        execution_data = {
            "executable": executable,
            "backend": "libinterflop_ieee.so (reference run)",
            "repetition": 1,
        }

        reference_run_data = read_probes_csv(temp.name, warnings, execution_data)[0]

        run_data["reference_value"] = reference_run_data["values"]
        run_data["check"] = run_data.apply(
            lambda x: validate_deterministic_probe(x), axis=1
        )

    else:
        run_data["reference_value"] = 0
        run_data["check"] = True

    deterministic_data.append(run_data)

    temp.close()


def run_tests(config):
    """
    Execute tests and collect results in a Pandas dataframe
    """

    # Run the build command
    print("Info [vfc_ci]: Building tests...")
    os.system(config["make_command"])

    # These are arrays of Pandas dataframes for now
    data = []
    deterministic_data = []

    # Only for non-deterministic data. Stored separately for now since it is
    # easier to add this to data after the groupby.
    checks_data = []

    # This will contain all executables/repetition numbers from which we could
    # not get any data
    warnings = []

    # Executables iteration
    for executable in config["executables"]:
        print("Info [vfc_ci]: Running executable :", executable["executable"], "...")

        parameters = ""
        if "parameters" in executable:
            parameters = executable["parameters"]

        # Backends iteration
        for backend in executable["vfc_backends"]:
            os.putenv("VFC_BACKENDS", backend["name"])

            command = "./" + executable["executable"] + " " + parameters

            # By default, we expect to have a number of repetitions specified
            # to run the tests in "non-deterministic" mode.
            if "repetitions" in backend:
                repetitions = backend["repetitions"]

                run_non_deterministic(
                    command,
                    repetitions,
                    executable["executable"],
                    backend["name"],
                    data,
                    checks_data,
                    warnings,
                )

            # However, if it is not specified, we'll assume a deterministic
            # backend and fall back to this mode (so as to avoid the same data
            # processing phase used for non-deterministic mode).
            else:
                # The run is executed in this helper function
                run_deterministic(
                    command,
                    executable["executable"],
                    backend["name"],
                    deterministic_data,
                    warnings,
                )

    # Make sure we have some data to work on
    assert len(data) != 0 or len(deterministic_data) != 0, (
        "Error [vfc_ci]: No data have been generated "
        "by your tests executions, aborting run without writing results file"
    )

    # Combine all separate executions in one dataframe
    if len(data) > 0:
        data = pd.concat(data, sort=False, ignore_index=True)
        data = (
            data.groupby(["test", "variable", "vfc_backend"])
            .values.apply(list)
            .reset_index()
        )

        data = data.set_index(["test", "variable", "vfc_backend"]).sort_index()
        checks_data = pd.concat(checks_data, sort=False, ignore_index=True)
        checks_data = checks_data.drop_duplicates(
            subset=["test", "variable", "vfc_backend"]
        )
        checks_data = checks_data.reset_index()
        del checks_data["index"]
        checks_data = checks_data.set_index(
            ["test", "variable", "vfc_backend"]
        ).sort_index()

        # Copy informations about accuracy thresholds to the main dataframe
        data["accuracy_threshold"] = checks_data["accuracy_threshold"]
        data["check_mode"] = checks_data["check_mode"]

    else:
        data = pd.DataFrame()

        # Combine all serparate executions of deterministic backends in one DF
    if len(deterministic_data) != 0:
        deterministic_data = pd.concat(
            deterministic_data, sort=False, ignore_index=True
        )

    else:
        deterministic_data = pd.DataFrame()

    return data, deterministic_data, warnings


def show_warnings(warnings):
    """
    Display all executions that resulted in a warning
    """

    if len(warnings) > 0:
        print(
            "Warning [vfc_ci]: Some of your runs could not generate any data "
            "(for instance because your code crashed) and resulted in "
            "warnings. Here is the complete list :",
            file=sys.stderr,
        )

        for i in range(0, len(warnings)):
            print("- Warning %s:" % (i + 1), file=sys.stderr)

            print("  Executable: %s" % warnings[i]["executable"], file=sys.stderr)
            print("  Backend: %s" % warnings[i]["backend"], file=sys.stderr)
            print("  Repetition: %s" % warnings[i]["repetition"], file=sys.stderr)


##########################################################################


def run(is_git_commit, export_raw_values, dry_run):
    """Entry point of vfc_ci test"""

    # Get config, metadata and data
    print("Info [vfc_ci]: Reading tests config file...")
    config = read_config()

    print("Info [vfc_ci]: Generating run metadata...")
    metadata = generate_metadata(is_git_commit)

    data, deterministic_data, warnings = run_tests(config)
    show_warnings(warnings)

    # Data processing
    if not data.empty:
        data = data_processing(data)

        # Link run timestamp
        data["timestamp"] = metadata["timestamp"]

    if not deterministic_data.empty:
        deterministic_data = deterministic_data.set_index(
            ["test", "variable", "vfc_backend"]
        ).sort_index()
        deterministic_data["timestamp"] = metadata["timestamp"]

    filename = metadata["hash"] if is_git_commit else str(metadata["timestamp"])

    # Prepare metadata for export
    metadata = pd.DataFrame.from_dict([metadata])
    metadata = metadata.set_index("timestamp")

    # WARNING : Exporting to HDF5 implicitly requires to install "tables" on the
    # system

    if not dry_run:
        # Export raw if needed
        if export_raw_values:
            data.to_hdf(filename + ".vfcraw.h5", key="data")
            metadata.to_hdf(filename + ".vfcraw.h5", key="metadata")
            deterministic_data.to_hdf(filename + ".vfcraw.h5", key="deterministic_data")

        # Export metadata
        metadata.to_hdf(filename + ".vfcrun.h5", key="metadata")

        # Export data
        if not data.empty:
            del data["values"]
        data.to_hdf(filename + ".vfcrun.h5", key="data")

        # Export deterministic data
        deterministic_data.to_hdf(filename + ".vfcrun.h5", key="deterministic_data")

    # Print termination messages
    print(
        "Info [vfc_ci]: The results have been successfully written to "
        "%s.vfcrun.h5." % filename
    )

    if export_raw_values:
        print(
            "Info [vfc_ci]: A file containing the raw values has also been "
            "created : %s.vfcraw.h5." % filename
        )

    if dry_run:
        print(
            "Info [vfc_ci]: The dry run flag was enabled, so no files were "
            "actually created."
        )
