#!/bin/sh

mkdir -p m4
cp $(interflop-config --m4dir)/*.m4 m4/
autoreconf -is
