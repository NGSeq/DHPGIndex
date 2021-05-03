#!/usr/bin/env bash

i=$(printf "%02d" $1)

HDFSPATH=$2
OUT=$3
REFS=$4 #How many genomes are used in dictionary parsing
let DRMEM=$5-$i #Spark driver memory
let EXMEM=$6-$i #Spark executor memory
let PARTITIONS=$7-$i #To how many partitions chromosomes are split
CORES=4
HDFSURL=$8
MASTER=$9

#spark.dynamicAllocation.minExecutors, spark.dynamicAllocation.maxExecutors, and spark.dynamicAllocation.initialExecutors spark.dynamicAllocation.executorAllocationRatio
#--conf spark.scheduler.allocation.file=/opt/spark/conf/fairscheduler.xml.template --conf spark.scheduler.pool=pool1

spark-submit --master yarn-client --conf spark.scheduler.minRegisteredResourcesRatio=0.3 --conf spark.executor.cores=$CORES --conf spark.dynamicAllocation.cachedExecutorIdleTimeout=240 --conf spark.dynamicAllocation.executorAllocationRatio=0.5 --conf spark.scheduler.mode=FIFO --conf spark.driver.memory=${DRMEM}g --conf spark.executor.memory=${EXMEM}g --conf spark.dynamicAllocation.enabled=true --conf spark.shuffle.service.enabled=true --conf spark.port.maxRetries=200 --conf spark.executor.memoryOverhead=15g --conf spark.driver.maxResultSize=6000m --conf spark.executor.heartbeatInterval=100 --conf spark.network.timeout=86400 --class org.ngseq.dhpgidx.DistributedRLZFasta ../target/dhpgidx-0.1-jar-with-dependencies.jar $1 ${HDFSPATH}/*.$i $HDFSURL $OUT $REFS $PARTITIONS &> DRLZFasta.$1.log
