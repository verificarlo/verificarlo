#!/bin/sh
autoreconf -is

rm -rf src/tools/sigdigits/
git submodule update --init --recursive