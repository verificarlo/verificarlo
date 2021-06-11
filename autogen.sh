#!/bin/sh
autoreconf -is

git submodule init
git submodule update
