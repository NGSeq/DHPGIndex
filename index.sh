#!/usr/bin/env bash

#./index.sh 2122 out FMI

set -e 
set -o pipefail

HDFS_PATH=$1
OUTPUT_FOLDER=$2
KERNEL=$3

source config.sh

PG_REF="${OUTPUT_FOLDER}/pangen.fa"
touch ${PG_REF}

JAR_PATH="target/scala-2.11/panquery_2.11-0.1.jar"
start=`date +%s`

#spark-submit --master yarn --deploy-mode client --conf spark.executor.memory=16g \
#--conf spark.driver.memory=20g --conf spark.driver.maxResultSize=5g --conf spark.executor.instances=21 --conf spark.executor.cores=3 \
#--conf spark.executor.memoryOverhead=2000 --class org.ngseq.panquery.DistributedRLZ \
#${JAR_PATH} ${HDFS_PATH} ${REF_SIZE} ${RLZ_SPLITS}
spark-submit --master yarn --deploy-mode client --conf spark.executor.memory=16g \
--conf spark.scheduler.mode=FAIR --conf spark.shuffle.service.enabled=true --conf spark.executor.memoryOverhead=1000 --conf spark.port.maxRetries=100 \
--conf spark.driver.memory=20g --conf spark.executor.instances=6 \
--conf spark.yarn.executor.memoryOverhead=1000 --class org.ngseq.panquery.DistributedRLZ \
${JAR_PATH} ${HDFS_PATH} ${REF_SIZE} ${RLZ_SPLITS}

#spark-submit --master yarn --deploy-mode client --num-executors 10 --executor-memory 10g --conf spark.driver.memory=20g --conf spark.executor.memoryOverhead=2000 --class org.ngseq.panquery.DistributedRLZ ${JAR_PATH} hdfs://m1.novalocal:8020/${HDFS_PATH} ${REF_SIZE} ${RLZ_SPLITS}

end=`date +%s`
runtime=$((end-start))
echo "rlz: ${runtime}" >> $LOG_FILE

#hdfs dfs -getmerge ${HDFS_PATH} ${PG_REF}

#mkdir -p tmp/pangen
#hdfs dfs -get ${HDFS_PATH} tmp/pangen

#add filenames to the beginning of each file
#names=($(ls tmp/pangen/${HDFS_PATH}))

#add fasta flags
#for i in ${names[@]}
#do
#        sed -i "1s/^/>$i\n/" tmp/pangen/${HDFS_PATH}/${i}
#done
#join
#cat tmp/pangen/${HDFS_PATH}/* >> ${INDEX_OUT}
#rm -f tmp/pangen/*

ORIG=$( pwd )

# change directory to run chic

DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
# last parameter should be over 100 (read size)
# save everything still to project root
start=`date +%s`
#./index/chic_index --threads=${THREADS} --kernel=${KERNEL} --verbose=5 --lz-input-file=${ORIG}/merged.lz ${PG_REF}  ${MATCH_LEN} 2>&1 | tee ${OUTPUT_FOLDER}/index.log
./index/build_index --threads=${THREADS} --kernel=${KERNEL} --verbose=2 --lz-input-file=${ORIG}/merged.lz ${PG_REF}  ${MATCH_LEN} 2>&1 | tee ${OUTPUT_FOLDER}/index.log
end=`date +%s`
runtime=$((end-start))
echo "index: ${runtime}" >> $LOG_FILE
