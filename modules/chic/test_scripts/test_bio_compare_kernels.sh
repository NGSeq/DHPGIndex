#!/usr/bin/env bash
set -o errexit
set -o nounset
set -o pipefail 

source "./utils.sh"
source "./bio_utils.sh"
DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

if [ "$#" -ne 3 ]; then
  echo "Script: '${0}'"
  echo "uses 3 params instead of $# " 
  exit 1
fi

utils_assert_file_exists ${1} 
utils_assert_file_exists ${2} 


REFERENCE=`readlink -f ${1}`
READS=`readlink -f ${2}`
OTHER_BUILD_FLAGS=${3}

VG_FLAGS='--vgdb-error=1 --leak-check=full --show-reachable=yes'
#PREPROCESSOR="gdb --args"
PREPROCESSOR="valgrind ${VG_FLAGS}"
#PREPROCESSOR=""
M=60

echo "*******************************"
echo "BWA (baseline) ::"
echo "*******************************"
BWA_BIN=${DIR}/../ext/BWA/bwa-0.7.12/bwa
BWA_BUILD_FLAGS="index"
utils_assert_file_exists ${BWA_BIN}
BWA_OUTPUT_ALL=${REFERENCE}.bwa_mathces.ALL.sam
BWA_OUTPUT_NONE=${REFERENCE}.bwa_mathces.NONE.sam
rm -f ${BWA_OUTPUT_ALL}
rm -f ${BWA_OUTPUT_NONE}
rm -f ${REFERENCE}.*

${BWA_BIN} ${BWA_BUILD_FLAGS} ${REFERENCE}

#utils_pause 'Press any key to continue with queries test...'
BWA_QUERY_FLAGS="mem -a"
${BWA_BIN} ${BWA_QUERY_FLAGS}  ${REFERENCE} ${READS} > ${BWA_OUTPUT_ALL}

BWA_QUERY_FLAGS="mem"
${BWA_BIN} ${BWA_QUERY_FLAGS}  ${REFERENCE} ${READS} > ${BWA_OUTPUT_NONE}

utils_assert_file_exists ${BWA_OUTPUT_ALL} 
utils_assert_file_exists ${BWA_OUTPUT_NONE} 

echo "*******************************"
echo "end of BWA"
echo "*******************************"

echo "********************************************************************"

echo "*******************************"
echo "Hybrid index::"
echo "*******************************"


HI_REPORT="ALL"  # TODO possible parameter.
for KERNEL in BOWTIE2 BWA
do
  HI_OUTPUT="HI_${KERNEL}_MATCHES.${HI_REPORT}"
  rm -f ${HI_OUTPUT}

  HI_BUILD_BIN=${DIR}/../src/build_index
  HI_BUILD_FLAGS="--kernel=${KERNEL} ${OTHER_BUILD_FLAGS}"
  utils_assert_file_exists ${HI_BUILD_BIN}
  ${PREPROCESSOR} ${HI_BUILD_BIN} ${HI_BUILD_FLAGS} ${REFERENCE} ${M}

  #utils_pause 'Press any key to continue with queries test...'
  HI_QUERY_BIN=${DIR}/../src/load_index
  utils_assert_file_exists ${HI_QUERY_BIN}

  SAM_ON_KERNEL="Reads_on_kernel.${KERNEL}.sam"

  HI_QUERY_FLAGS="--input=FQ  --output=${HI_OUTPUT} --secondary=${HI_REPORT}"
  ${PREPROCESSOR} ${HI_QUERY_BIN} ${HI_QUERY_FLAGS}  ${REFERENCE} ${READS}
  cp .reads_aligned_to_kernel.sam ${SAM_ON_KERNEL}
  bio_validate_outputSAM_inputFQ ${HI_OUTPUT} ${READS}

  #utils_pause 'Press any key to continue with BWA baseline ..."'

  #utils_pause 'Press any key to continue with next test: EXTERNAL MEMORY:'
  utils_assert_file_exists ${HI_OUTPUT} 
done


LIST="HI_BWA_MATCHES.${HI_REPORT} HI_BOWTIE2_MATCHES.${HI_REPORT} ${BWA_OUTPUT_ALL} ${BWA_OUTPUT_NONE}"
utils_assert_list_of_files_exists ${LIST}
utils_ask_to_run_command "vimdiff ${LIST}"

LIST="Reads_on_kernel.BWA.sam Reads_on_kernel.BOWTIE2.sam"
utils_assert_list_of_files_exists ${LIST}
utils_ask_to_run_command "vimdiff ${LIST}"
