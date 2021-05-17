#!/usr/bin/env bash

HDFSFASTAPATH=$1
HDFSDRLZPATH=$2
LOCALOUT=$3
DRMEM=5g
EXMEM=2g
MAXQLEN=$4 #Maximum number of sequences to use in dictionary, useful when there is lots of short sequences
HDFSURL=$5
MASTER=$6 #For single node testing use local[n] where n is the amount of executors
QSEQS=$7
THREADS=$8


spark-submit --master $MASTER --conf spark.executor.memory=$EXMEM --conf spark.shuffle.service.enabled=true --conf spark.shuffle.blockTransferService=nio --conf spark.executor.cores=4 --conf spark.executor.memoryOverhead=1g --conf spark.scheduler.mode=FAIR --conf spark.driver.memory=$DRMEM --conf spark.dynamicAllocation.enabled=true --conf spark.port.maxRetries=100 --conf spark.driver.maxResultSize=10000m --conf spark.executor.heartbeatInterval=100 --conf spark.dynamicAllocation.executorIdleTimeout=320 --conf spark.network.timeout=5000 --class org.ngseq.dhpgidx.DistributedIndexing ../target/dhpgidx-0.1-jar-with-dependencies.jar --threads $THREADS --in $HDFSFASTAPATH --lzin $HDFSDRLZPATH --local $LOCALOUT --qlen $MAXQLEN --hdfs $HDFSURL --qfile $QSEQS #&> dist_index_and_align.log

echo "spark-submit --master $MASTER --conf spark.executor.memory=$EXMEM --conf spark.shuffle.service.enabled=true --conf spark.shuffle.blockTransferService=nio --conf spark.executor.cores=4 --conf spark.executor.memoryOverhead=1g --conf spark.scheduler.mode=FAIR --conf spark.driver.memory=$DRMEM --conf spark.dynamicAllocation.enabled=true --conf spark.port.maxRetries=100 --conf spark.driver.maxResultSize=10000m --conf spark.executor.heartbeatInterval=100 --conf spark.dynamicAllocation.executorIdleTimeout=320 --conf spark.network.timeout=5000 --class org.ngseq.dhpgidx.DistributedIndexing ../target/dhpgidx-0.1-jar-with-dependencies.jar --threads $THREADS --in $HDFSFASTAPATH --lzin $HDFSDRLZPATH --local $LOCALOUT --qlen $MAXQLEN --hdfs $HDFSURL --qfile $QSEQS
"
