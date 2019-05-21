#!/usr/bin/env bash
set -e 
set -o pipefail

source config.sh

OUTPUT_FOLDER=$1
INDEX=$2
N_REFS=$3
TMP_PATH=$4
SPLIT=$5
SAMS=$6
READS_1=$7
READS_2=""
if [ $SPLIT -eq 1 ]
then
READS_2=$8
fi

SAM_PATH="${OUTPUT_FOLDER}/align.sam"
TMP_SAM="${OUTPUT_FOLDER}/out.sam"

JAR_PATH="pangenomics-0.9-jar-with-dependencies.jar"
start=`date +%s`
spark-submit --master yarn --deploy-mode client --conf spark.executor.memory=5g \
--conf spark.driver.memory=20g --conf spark.executor.instances=21 --conf spark.yarn.executor.memoryOverhead=2000 \
--class fi.aalto.ngs.seqspark.pangenomics.ParallelAlign ${JAR_PATH} "${INDEX}/*" "/tmp/out/index" \
${OUTPUT_FOLDER} ${N_REFS} ${SPLIT} ${THREADS_DIS} ${READS_1} ${READS_2} ${SAMS}
end=`date +%s`
runtime=$((end-start))
echo "align: ${runtime}" >> $LOG_FILE
# align
DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

head -n3 $TMP_SAM > "${OUTPUT_FOLDER}/tmp.sam"
sed '/^#/ d' $TMP_SAM >> "${OUTPUT_FOLDER}/tmp.sam"
mv "${OUTPUT_FOLDER}/tmp.sam" "${OUTPUT_FOLDER}/out.sam"

# convert bowtie sam file into hybrid index sam file
${DIR}/chic_map --threads=${THREADS} -o ${SAM_PATH} ${OUTPUT_FOLDER}/pangen.fa ${TMP_SAM}

shopt -s nullglob dotglob
TMP_FILES=(${TMP_PATH}/*)
start=`date +%s`

task() {
  CURRENT_REFERENCE=$1
  OUTPUT_FOLDER=$2
  SAM_PATH=$3
  # Make the calls
  CUR_NAME=$(basename -- "$CURRENT_REFERENCE")
  CUR="${CUR_NAME%.*}"
  SAM_FILE=${OUTPUT_FOLDER}/${CUR}.sam.gz
  cat ${SAM_PATH} | grep -P "${CUR}\t" | gzip > ${SAM_FILE}	
}

start=`date +%s`
N=${PAR_PROCESSES}
open_sem $N
echo "Splitting into many SAM FILES:"
for CURRENT_REFERENCE in "${TMP_FILES[@]}"
do
	run_with_lock task $CURRENT_REFERENCE $OUTPUT_FOLDER $SAM_PATH
done
wait
end=`date +%s`
runtime=$((end-start))
echo "sams: ${runtime}" >> $LOG_FILE

echo "SAM files generated"


