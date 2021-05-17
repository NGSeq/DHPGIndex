#!/usr/bin/env bash

HDFSPATH=$1
HDFSLZOUT=$2
HDFSFASTAOUT=$3
DICTSIZE=$4 #Size of dictionary in decimals (maximum is 1.0)
CORES=4
MAXDICTREFS=1000 #Maximum number of sequences in dictionary, useful when there is lots of short sequences
HDFSURL=$5
MASTER=$6
DRMEM=$7
EXMEM=$8

spark-submit --master $MASTER --conf spark.executor.memory=$EXMEM --conf spark.shuffle.service.enabled=true --conf spark.shuffle.blockTransferService=nio --conf spark.executor.cores=$CORES --conf spark.executor.memoryOverhead=1g  --conf spark.scheduler.mode=FAIR --conf spark.driver.memory=$DRMEM  --conf spark.dynamicAllocation.enabled=true --conf spark.port.maxRetries=100 --conf spark.driver.maxResultSize=10000m --conf spark.executor.heartbeatInterval=100 --conf spark.dynamicAllocation.executorIdleTimeout=320 --conf spark.network.timeout=5000 --class org.ngseq.dhpgidx.DistributedRLZGroupTax ../target/dhpgidx-0.1-jar-with-dependencies.jar $HDFSPATH $HDFSURL $HDFSLZOUT $DICTSIZE $MAXDICTREFS $HDFSFASTAOUT #&> DRLZGroupTax.log
