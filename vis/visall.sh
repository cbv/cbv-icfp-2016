#!/bin/bash

FILES=../problems/prob*
for f in $FILES
do
    echo "Outputing $f"
    ./vis $f ../images/${f##*/}.bmp
done