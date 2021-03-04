#!/usr/bin/env bash
set -e 
set -o pipefail

CHR=$1
HDFSPGPATH=$2
HDFSLZPATH=$2drlz
LOCALPATH=/mnt/tmp
ALIGNER=BOWTIE2
MAX_QUERY_LEN=102
cpus=$(lscpu -p | wc -l)

echo Executing index_chr.sh $1 $2 with $cpus cpus  

    i=$(printf "%02d" $CHR)
    hdfs dfs -getmerge $HDFSLZPATH/*.$i $LOCALPATH/chr$i.lz
    /opt/chic/src/chic_index --threads=${cpus}  --kernel=${ALIGNER} --verbose=2 --lz-input-file=${LOCALPATH}/chr${i}.lz ${LOCALPATH}/chr${i}.fa ${MAX_QUERY_LEN}

