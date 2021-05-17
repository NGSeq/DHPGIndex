#!/usr/bin/env bash
set -e 
set -o pipefail

NODE=$1
HDFSPGPATH=$2
HDFSLZPATH=$3
LOCALPATH=/mnt/tmp
ALIGNER=BLAST
MAX_QUERY_LEN=1000
cpus=$(lscpu -p | wc -l)

echo Executing index_partition.sh $1 $2 with $cpus cpus

    i=$(printf "%02d" $NODE)
    hdfs dfs -getmerge $HDFSLZPATH/$i $LOCALPATH/part-${i}.lz&
    hdfs dfs -getmerge $HDFSPGPATH/$i $LOCALPATH/part-${i}.fa
    /opt/dhpgindex/chic_index --threads=${cpus}  --kernel=${ALIGNER} --verbose=2 --lz-input-file=${LOCALPATH}/part-${i}.lz ${LOCALPATH}/part-${i}.fa ${MAX_QUERY_LEN} &> chic_index$1.log

