#!/usr/bin/env bash

seq 1 3 | xargs -I{} -n 1 -P 3 ./drlz_hg.sh {} pg/ drlz/ 75 40 80 30&
seq 4 7 | xargs -I{} -n 1 -P 4 ./drlz_hg.sh {} pg/ drlz/ 75 40 70 30&
seq 8 11| xargs -I{} -n 1 -P 4 ./drlz_hg.sh {} pg/ drlz/ 75 40 60 30&
seq 12 16| xargs -I{} -n 1 -P 5 ./drlz_hg.sh {} pg/ drlz/ 75 40 50 30&
seq 17 22| xargs -I{} -n 1 -P 6 ./drlz_hg.sh {} pg/ drlz/ 75 40 40 30&



