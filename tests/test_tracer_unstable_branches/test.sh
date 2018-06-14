#!/bin/bash

rm -f locationInfo.map
make dot
veritracer launch --jobs=16 --binary="dot_product 10" --force
veritracer analyze --filename veritracer.dat
HASH=$(grep ret locationInfo.map | grep naive_dot_product | cut -d':' -f1)
veritracer plot .vtrace/veritracer.000bt --no-show --output=dot_product.png  --invocation-mode --transparency=0.5 -v $HASH -bt=.vtrace/.backtrace/000bt.rdc
