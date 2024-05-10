#!/bin/sh

mkdir -p m4
for i in src/backends/interflop-backend-*; do mkdir -p $i/m4; done

autoreconf -is -I $(realpath src/interflop-stdlib/m4/)
