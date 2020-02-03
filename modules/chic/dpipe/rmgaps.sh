#!/usr/bin/env bash

set -o errexit

export SHELL=$(type -p bash)

#mkdir -p /mnt/tmp/gaps/
task(){
    #/opt/chic/store_gaps $1 /media/tmp/all/gaps/
    FNAME=$(basename -- "$1")
    #echo ">$FNAME" >> /mnt/tmp/pg.fa
    #echo >> /mnt/tmp/pg.fa
    cat $1 | tr -d '-' >> /media/genomes/gapless/${FNAME}b
}
export -f task

ls /media/tmp/all/all/* | parallel -j30 task {}
hdfs dfs -put /media/genomes/gapless/

