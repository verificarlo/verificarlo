#!/bin/sh

export VFC_BACKENDS_SILENT_LOAD="True"
export VFC_BACKENDS_LOGGER="False"

vfc_ci test

if ls *.vfcrun.h5; then
    echo "Run file found, SUCCESS"
    exit 0
else
    echo "Run file not found, FAILURE"
    exit 1
fi
