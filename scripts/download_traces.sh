#!/bin/bash

i=0
while read LINE
do
    echo "axel -n 16 -a http://hpca23.cse.tamu.edu/champsim-traces/speccpu/$LINE"
done < selected
