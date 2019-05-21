#!/usr/bin/env bash
set -e 
set -o pipefail

HDFS_PATH=$1
INPUT_PATH=$2
OUTPUT_FOLDER=$3
TMP=$4

source config.sh

JAR_PATH="pangenomics-0.9-jar-with-dependencies.jar"
start=`date +%s`
spark-submit --master yarn --deploy-mode client --conf spark.executor.memory=16g \
--conf spark.driver.memory=20g --conf spark.executor.instances=21 --conf spark.executor.cores=3 \
--conf spark.yarn.executor.memoryOverhead=2000 --class fi.aalto.ngs.seqspark.pangenomics.DistributedRLZ \
${JAR_PATH} ${HDFS_PATH} ${REF_SIZE} ${RLZ_SPLITS}
end=`date +%s`
runtime=$((end-start))
echo "rlz: ${runtime}" >> $LOG_FILE

# copy local

# create virtualenv
python create_fasta.py -i ${INPUT_PATH} -p ${TMP} -o ${OUTPUT_FOLDER}/pangen.fa
truncate -s -1 ${OUTPUT_FOLDER}/pangen.fa

# create index

ORIG=$( pwd )

# change directory to run chic

DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )


INDEX_OUT="${OUTPUT_FOLDER}/pangen.fa"
# last parameter should be over 100 (read size)
# save everything still to project root
start=`date +%s`
${DIR}/chic_index --threads=${THREADS} --kernel=BOWTIE2 --lz-input-file=${ORIG}/merged.lz ${INDEX_OUT}  ${MATCH_LEN} 2>&1 | tee ${OUTPUT_FOLDER}/index.log
end=`date +%s`
runtime=$((end-start))
echo "index: ${runtime}" >> $LOG_FILE
