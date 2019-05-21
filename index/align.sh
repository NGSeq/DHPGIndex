#!/usr/bin/env bash
set -e 
set -o pipefail

OUTPUT_FOLDER=$1
INDEX_OUT=$2
N_REFS=$3
TMP_PATH=$4
READS_1=$5
READS_2=$6

source config.sh

SAM_PATH="${OUTPUT_FOLDER}/align.sam"
# align
DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

start=`date +%s`
${DIR}/chic_align -v2 -t ${THREADS} -o ${SAM_PATH} ${INDEX_OUT}/pangen.fa ${READS_1} ${READS_2}
end=`date +%s`
runtime=$((end-start))
echo "align: ${runtime}" >> $LOG_FILE

shopt -s nullglob dotglob
TMP_FILES=(${TMP_PATH}/*)

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


