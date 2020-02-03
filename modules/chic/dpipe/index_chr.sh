#!/usr/bin/env bash
set -e 
set -o pipefail

CHR=$1
HDFSPGPATH=$2
HDFSLZPATH=$2drlz
LOCALPATH=/mnt/tmp
cpus=$(lscpu -p | tail -n+5 | wc -l)

echo Executing index_chr.sh $1 $2 with $cpus cpus  

    i=$(printf "%02d" $CHR)
    hdfs dfs -getmerge $HDFSLZPATH/*.$i.pos $LOCALPATH/chr$i.lz&
    hdfs dfs -get /user/root/reads_1/* $LOCALPATH/reads_1.fq&
    hdfs dfs -get /user/root/reads_2/* $LOCALPATH/reads_2.fq&
    #hdfs dfs -text 10gapless/chr$i.fa/* | tr -d '-' > $LOCALPATH/chr$i.fa
    hdfs dfs -getmerge $2gapless/*$i* $LOCALPATH/chr$i.fa
    #/opt/chic/store_gaps /mnt/tmp/chr$i.fa
    /opt/chic/src/chic_index --threads=$cpus  --kernel=BOWTIE2 --verbose=1 --lz-input-file=$LOCALPATH/chr$i.lz $LOCALPATH/chr$i.fa 80
    /opt/chic/src/chic_align -v1 -t $cpus -o $LOCALPATH/aligned.sam /mnt/tmp/chr$i.fa $LOCALPATH/reads_1.fq $LOCALPATH/reads_2.fq
    /opt/samtools/samtools view -F4 $LOCALPATH/aligned.sam > $LOCALPATH/mapped$i.sam
    #scp -o "StrictHostKeyChecking no" node-$1:/mnt/tmp/mapped$i.sam sams/
