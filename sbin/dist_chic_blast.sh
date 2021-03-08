#!/usr/bin/env bash
set -e 
set -o pipefail

align() {
    i=$(printf "%02d" $1)

    ssh  -tt -o "StrictHostKeyChecking no" node-$1 hdfs dfs -getmerge qseqs/ /mnt/tmp/qseqs.fa
    ssh  -tt -o "StrictHostKeyChecking no" node-$1 /opt/chic/index/chic_align -v2 -t 16 -o /mnt/tmp/aligned-$i /mnt/tmp/part-$i.fa /mnt/tmp/qseqs.fa &> chic_align$i.log
    scp -o "StrictHostKeyChecking no" node-1:/mnt/tmp/aligned-$i /mnt/tmp/aligned-$i

}

for num in {1..22}; do align "$num"& done

