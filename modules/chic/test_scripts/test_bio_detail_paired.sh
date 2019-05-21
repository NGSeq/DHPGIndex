#!/usr/bin/env bash
set -o errexit
set -o nounset
set -o pipefail 

source "./utils.sh"
source "./bio_utils.sh"
DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

if [ "$#" -ne 5 ]; then
  echo "Script: '${0}'"
  echo "uses 5 params instead of $# " 
  exit 1
fi

utils_assert_file_exists ${1} 
utils_assert_file_exists ${2} 


REFERENCE=`readlink -f ${1}`
READS_1=`readlink -f ${2}`
READS_2=`readlink -f ${3}`
OTHER_BUILD_FLAGS=${4}
EXPECTED_MATCHES=${5}

MY_M=60
#cd ..;



HI_OUTPUT=HI_MATHCES.in_IM
rm -f ${HI_OUTPUT}
rm -f ${REFERENCE}.*

HI_BUILD_BIN=${DIR}/../src/build_index
HI_BUILD_FLAGS="--verbose=${VERBOSE_LEVEL}  ${OTHER_BUILD_FLAGS}"

HI_QUERY_BIN=${DIR}/../src/load_index
HI_QUERY_FLAGS="--verbose=${VERBOSE_LEVEL} --input=FQ  --output=${HI_OUTPUT} -t4"
if [[ ${HI_BUILD_FLAGS} == *"BOWTIE"* ]]; then
  HI_QUERY_FLAGS="${HI_QUERY_FLAGS} --kernel-options=--very-sensitive"
  HI_QUERY_FLAGS="${HI_QUERY_FLAGS} --kernel-options=--np=8"
  HI_QUERY_FLAGS="${HI_QUERY_FLAGS} --kernel-options=--n-ceil=L,0,0.02"
fi
utils_assert_file_exists ${HI_BUILD_BIN}
utils_assert_file_exists ${HI_QUERY_BIN}
echo "*******************************"
echo "Hybrid index::"
echo "*******************************"
echo "*******************************"
${PREPROCESSOR} ${HI_BUILD_BIN} ${HI_BUILD_FLAGS} ${REFERENCE} ${MY_M}
#utils_pause 'Press any key to continue with queries test...'
${PREPROCESSOR} ${HI_QUERY_BIN} ${HI_QUERY_FLAGS}  ${REFERENCE} ${READS_1} ${READS_2}
#TODO:
#bio_validate_outputSAM_inputFQ ${HI_OUTPUT} ${READS}

utils_assert_file_exists ${HI_OUTPUT} 

bio_validate_expected_matches ${HI_OUTPUT} ${EXPECTED_MATCHES}
