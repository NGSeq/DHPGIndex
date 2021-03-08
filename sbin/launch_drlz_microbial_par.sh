#!/usr/bin/env bash

seq 1 25 | xargs -I{} -n 1 -P 5 ./drlz_microbes.sh {} pg lz groupedfasta 0.33 &




