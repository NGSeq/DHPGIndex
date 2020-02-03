#!/usr/bin/env bash

seq 1 22 | parallel -j4 ./runp2.sh {}
#seq 1 6 | xargs -I{} -n 1 -P 3 -a arglist.txt ./t.sh $1 $2 {}
