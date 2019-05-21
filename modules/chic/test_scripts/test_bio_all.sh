#!/usr/bin/env bash
set -o errexit
set -o nounset
set -o pipefail 

source "./utils.sh"
source "./bio_utils.sh"
DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

if [ "$#" -ne 4 ]; then
  echo "Script: '${0}'"
  echo "uses 3 params instead of $# " 
  exit 1
fi

utils_assert_file_exists ${1} 
utils_assert_file_exists ${2} 


REFERENCE=`readlink -f ${1}`
READS=`readlink -f ${2}`
KERNEL=${3}
OTHER_BUILD_FLAGS=${4}

#cd ..;


VG_FLAGS='--vgdb-error=1 --leak-check=full --show-reachable=yes'
PREPROCESSOR="valgrind ${VG_FLAGS}"
#PREPROCESSOR=""


M=60

HI_OUTPUT_ALL=HI_MATHCES.ALL
HI_OUTPUT_LZ=HI_MATHCES.LZ
HI_OUTPUT_NONE=HI_MATHCES.NONE
BWA_OUTPUT_ALL=${REFERENCE}.bwa_mathces.ALL.sam
BWA_OUTPUT_NONE=${REFERENCE}.bwa_mathces.NONE.sam
rm -f ${HI_OUTPUT_ALL}
rm -f ${HI_OUTPUT_LZ}
rm -f ${HI_OUTPUT_NONE}
rm -f ${BWA_OUTPUT_ALL}
rm -f ${BWA_OUTPUT_NONE}
rm -f ${REFERENCE}.*

HI_BUILD_BIN=${DIR}/../src/build_index
HI_BUILD_FLAGS="--kernel=${KERNEL} ${OTHER_BUILD_FLAGS}"
utils_assert_file_exists ${HI_BUILD_BIN}
echo "*******************************"
echo "Hybrid index::"
echo "*******************************"
echo "*******************************"
${PREPROCESSOR} ${HI_BUILD_BIN} ${HI_BUILD_FLAGS} ${REFERENCE} ${M}

#utils_pause 'Press any key to continue with queries test...'
HI_QUERY_BIN=${DIR}/../src/load_index
utils_assert_file_exists ${HI_QUERY_BIN}

HI_QUERY_FLAGS="--input=FQ  --output=${HI_OUTPUT_ALL} --secondary=ALL"
${PREPROCESSOR} ${HI_QUERY_BIN} ${HI_QUERY_FLAGS}  ${REFERENCE} ${READS}
#cp .reads_aligned_to_kernel.sam ReadsToKernelALL.sam

HI_QUERY_FLAGS="--input=FQ  --output=${HI_OUTPUT_LZ} --secondary=LZ"
${PREPROCESSOR} ${HI_QUERY_BIN} ${HI_QUERY_FLAGS}  ${REFERENCE} ${READS}

HI_QUERY_FLAGS="--input=FQ  --output=${HI_OUTPUT_NONE} --secondary=NONE"
${PREPROCESSOR} ${HI_QUERY_BIN} ${HI_QUERY_FLAGS}  ${REFERENCE} ${READS}


#utils_pause 'Press any key to continue with BWA baseline ..."'

BWA_BIN=${DIR}/../ext/BWA/bwa-0.7.12/bwa
BWA_BUILD_FLAGS="index"
utils_assert_file_exists ${BWA_BIN}

echo "*******************************"
echo "BWA ::"
echo "*******************************"
echo "*******************************"
${BWA_BIN} ${BWA_BUILD_FLAGS} ${REFERENCE}

#utils_pause 'Press any key to continue with queries test...'
BWA_QUERY_FLAGS="mem -a"
${BWA_BIN} ${BWA_QUERY_FLAGS}  ${REFERENCE} ${READS} > ${BWA_OUTPUT_ALL}

BWA_QUERY_FLAGS="mem"
${BWA_BIN} ${BWA_QUERY_FLAGS}  ${REFERENCE} ${READS} > ${BWA_OUTPUT_NONE}

############################################
#BWA_BIN=

#utils_pause 'Press any key to continue with next test: EXTERNAL MEMORY:'
utils_assert_file_exists ${HI_OUTPUT_ALL} 
utils_assert_file_exists ${HI_OUTPUT_NONE} 
utils_assert_file_exists ${HI_OUTPUT_LZ} 
utils_assert_file_exists ${BWA_OUTPUT_ALL} 
utils_assert_file_exists ${BWA_OUTPUT_NONE} 

bio_validate_outputSAM_inputFQ ${HI_OUTPUT_NONE} ${READS}
bio_validate_outputSAM_inputFQ ${HI_OUTPUT_LZ} ${READS}
bio_validate_outputSAM_inputFQ ${HI_OUTPUT_ALL} ${READS}
COMM="vimdiff ${HI_OUTPUT_ALL} ${HI_OUTPUT_NONE} ${HI_OUTPUT_LZ} ${BWA_OUTPUT_ALL} ${BWA_OUTPUT_NONE}"
utils_ask_to_run_command "${COMM}"
