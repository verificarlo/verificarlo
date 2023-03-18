#!/bin/bash

# This script installs the interflop-stdlib library.

ROOT=$PWD # Save the current directory.

# This function checks the exit status of the last command.
# If it's not 0 (which means the command failed),
# it prints an error message and exits the script.
function check_last_command() {
    if [[ $? != 0 ]]; then
        echo "The last command failed. Please check the error message above."
        exit 1
    fi
}

# This function changes the current directory to the one specified as an argument.
function change_directory() {
    cd $1
    check_last_command
}

# This function runs the autogen.sh script.
function run_autogen() {
    ./autogen.sh
    check_last_command
}

# This function runs the configure script with the specified arguments.
function run_configure() {
    ./configure $@
    check_last_command
}

# This function runs the make command.
function build_project() {
    make
    check_last_command
}

# This function runs the "make install" command.
function install_project() {
    make install
    check_last_command
}

# This function installs the interflop-stdlib library.
function install_stdlib() {
    echo "Installing stdlib"
    echo "Arguments: $@"
    change_directory src/interflop-stdlib
    run_autogen
    run_configure --enable-warnings $@
    build_project
    install_project
    change_directory ${ROOT}
}

# Check if -h or --help was passed as an argument.
if [[ $1 == "-h" || $1 == "--help" ]]; then
    echo "Usage: $0 [configure arguments]"
    echo -e "\tAccepts all the arguments accepted by the configure script."
    echo -e "\tUse --prefix to specify a local installation directory."
    exit 0
fi

install_stdlib $@
