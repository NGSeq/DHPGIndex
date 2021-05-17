#!/usr/bin/env bash
#set -e
#set -o pipefail

CHR=$1
HDFSPGPATH=$2
HDFSLZPATH=$3
mode=$4
LOCALPATH=/mnt/tmp
ALIGNER=BOWTIE2
MAX_QUERY_LEN=$5
#threads=$(lscpu -p | wc -l)
threads=16

echo Executing index_chr.sh $1 $2 with $cpus cpus  

    i=$(printf "%02d" $CHR)
    hdfs dfs -getmerge $HDFSLZPATH/*.$i $LOCALPATH/part-${i}.lz&
    hdfs dfs -getmerge $HDFSPGPATH/*.$i $LOCALPATH/part-${i}.fa
    /opt/dhpgindex/chic_index --threads=${threads}  --kernel=${ALIGNER} --verbose=2 $mode --lz-input-plain-file=${LOCALPATH}/part-${i}.lz ${LOCALPATH}/part-${i}.fa ${MAX_QUERY_LEN} &> chic_index$1.log
