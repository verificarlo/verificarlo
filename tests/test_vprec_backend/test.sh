#!/bin/bash
set -e

# Build binaries
make all

# Extract pre-generated references
make mpfr_reference

# Check vprec backend against mpfr_reference
./check.sh

exit $?
