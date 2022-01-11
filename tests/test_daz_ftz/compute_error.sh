#!/usr/bin/env bash

TYPE=$1
BIN=$2
BACKEND=$3

TMPDIR=$(mktemp -d -p .)
cd $TMPDIR

REF=../ref_${TYPE^^}

IFS=" "
for OPTION in " " "--daz" "--ftz" "--daz --ftz"; do
    export VFC_BACKENDS="${BACKEND} --mode=ieee ${OPTION}"
    while read x y op; do
        $BIN "$x" "$y" "${op}" >>log_${TYPE^^}
    done <../value.${TYPE^^}
done

diff -I '#.*' log_${TYPE^^} ${REF}

echo $? >output.txt
