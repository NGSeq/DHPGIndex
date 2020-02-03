#!/usr/bin/env bash

#seq 1 22 | parallel -j3 ./runp2.sh {}
#1-5 40g 35g
#6-22 15g 15g

CHR=$1
INPATH=$2
DRLZOUT=$2drlz
GAPLESSOUT=$2gapless

hdfs dfs -mkdir -p $DRLZOUT 
hdfs dfs -mkdir -p $GAPLESSOUT

i=$(printf "%02d" $CHR)
echo Submitting job with args $i $INPATH/*.${i} $DRLZOUT 

spark-submit --master yarn-client --conf spark.executor.memory=35g --conf spark.driver.memory=35g --num-executors 32 --conf spark.dynamicAllocation.enabled=true --conf spark.shuffle.service.enabled=true --conf spark.port.maxRetries=100 --conf spark.broadcast.compress=false --conf spark.executor.memoryOverhead=1000 --conf spark.driver.maxResultSize=6000m --conf spark.executor.heartbeatInterval=500 --conf spark.network.timeout=1000 --class fi.aalto.ngs.seqspark.pangenomics.DistributedRLZGaps ../pangenomics-0.9-jar-with-dependencies.jar /media/msa/hg19/UCASE/radix$1 /media/msa/hg19/UCASE/chr${1}_uc.fa $INPATH/*.${i} hdfs://node-1.novalocal:8020 $DRLZOUT $GAPLESSOUT/chr${i}.fa 2&> log$1
