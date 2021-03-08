#!/usr/bin/env bash
set -e 
set -o pipefail

CHR=$1
HDFSPGPATH=$2
HDFSLZPATH=$3
mode=$4
LOCALPATH=/mnt/tmp
ALIGNER=BOWTIE2
MAX_QUERY_LEN=102
cpus=$(lscpu -p | wc -l)

echo Executing index_chr.sh $1 $2 with $cpus cpus  

    i=$(printf "%02d" $CHR)
    hdfs dfs -getmerge $HDFSLZPATH/*.$i $LOCALPATH/chr$i.lz&
    hdfs dfs -getmerge $HDFSPGPATH/*.$i $LOCALPATH/chr$i.fa
    /opt/chic/index/chic_index --threads=${cpus}  --kernel=${ALIGNER} --verbose=2 $mode --lz-input-file=${LOCALPATH}/chr${i}.lz ${LOCALPATH}/chr${i}.fa ${MAX_QUERY_LEN} &> chic_index.log

