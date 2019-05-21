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


FLAGS='--vgdb-error=1 --leak-check=full --show-reachable=yes'
PREPROCESSOR="valgrind ${FLAGS}"
#PREPROCESSOR=""


M=60

HI_OUTPUT=HI_MATHCES.in_EM
BWA_OUTPUT=${REFERENCE}.bwa_mathces.sam
rm -f ${HI_OUTPUT}
rm -f ${BWA_OUTPUT}
rm -f ${REFERENCE}.*

HI_BUILD_BIN=${DIR}/../src/build_index
HI_BUILD_FLAGS="--kernel=${KERNEL} --lz-parsing-method=EM ${OTHER_BUILD_FLAGS}"

HI_QUERY_BIN=${DIR}/../src/load_index
HI_QUERY_FLAGS="--input=FQ  --output=${HI_OUTPUT}"
utils_assert_file_exists ${HI_BUILD_BIN}
utils_assert_file_exists ${HI_QUERY_BIN}
echo "*******************************"
echo "Hybrid index::"
echo "*******************************"
echo "*******************************"
${PREPROCESSOR} ${HI_BUILD_BIN} ${HI_BUILD_FLAGS} ${REFERENCE} ${M}
#utils_pause 'Press any key to continue with queries test...'
${PREPROCESSOR} ${HI_QUERY_BIN} ${HI_QUERY_FLAGS}  ${REFERENCE} ${READS}
bio_validate_outputSAM_inputFQ ${HI_OUTPUT} ${READS}


BWA_BIN=${DIR}/../ext/BWA/bwa-0.7.12/bwa
BWA_BUILD_FLAGS="index"
BWA_QUERY_FLAGS="mem"
utils_assert_file_exists ${BWA_BIN}

echo "*******************************"
echo "BWA ::"
echo "*******************************"
echo "*******************************"
${BWA_BIN} ${BWA_BUILD_FLAGS} ${REFERENCE}
#utils_pause 'Press any key to continue with queries test...'
${BWA_BIN} ${BWA_QUERY_FLAGS}  ${REFERENCE} ${READS} > ${BWA_OUTPUT}

############################################
#BWA_BIN=

#utils_pause 'Press any key to continue with next test: EXTERNAL MEMORY:'
utils_assert_file_exists ${HI_OUTPUT} 
utils_assert_file_exists ${BWA_OUTPUT} 

COMM="vimdiff ${HI_OUTPUT} ${BWA_OUTPUT}"
utils_ask_to_run_command "${COMM}"

