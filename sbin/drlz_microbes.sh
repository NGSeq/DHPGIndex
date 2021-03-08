#!/usr/bin/env bash


PARTITION=$1
HDFSPATH=$2
HDFSLZOUT=$3
HDFSFASTAOUT=$4
DICTQUOTIENT=$5  #How large quotient of all genomes are used in dictionary parsing (1/DICTQUOTIENT)
DRMEM=30g
EXMEM=40g
CORES=4
MAXDICTREFS=100000 #Maximum amount of sequence to use in dictionary, useful when there is lots of short sequences
HDFSURL="hdfs://node-1.novalocal:8020"

i=$(printf "%02d" $PARTITION)

spark-submit --master yarn --deploy-mode client --conf spark.executor.memory=$EXMEM --conf spark.shuffle.service.enabled=true --conf spark.shuffle.blockTransferService=nio --conf spark.executor.cores=$CORES --conf spark.executor.memoryOverhead=10g  --conf spark.scheduler.mode=FAIR --conf spark.driver.memory=$DRMEM  --conf spark.dynamicAllocation.enabled=true --conf spark.port.maxRetries=100 --conf spark.driver.maxResultSize=10000m --conf spark.executor.heartbeatInterval=100 --conf spark.dynamicAllocation.executorIdleTimeout=320 --conf spark.network.timeout=5000 --class org.ngseq.dhpgidx.DistributedRLZGroupTax ../target/dhpgidx-0.1-jar-with-dependencies.jar $HDFSPATH/$i $HDFSURL $HDFSLZOUT/$i $DICTQUOTIENT $MAXDICTREFS $HDFSFASTAOUT &> DRLZGroupTax.$i.log
