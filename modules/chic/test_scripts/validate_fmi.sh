#!/usr/bin/env bash
set -o errexit
set -o nounset
set -o pipefail

source "./utils.sh"
DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
HOST="$(hostname)"


UTEST_FOLDER="${DIR}/utest_data"
if [[ ${HOST} =~ hp8x-37 || ${HOST} =~ whq ]]; then
  FILE_LIST=( "${UTEST_FOLDER}/file4.txt" )
  #FILE_LIST=( "${UTEST_FOLDER}/file1.txt" )
  #FILE_LIST=("${UTEST_FOLDER}/file1.txt" "${UTEST_FOLDER}/file2.txt" "${UTEST_FOLDER}/file3.txt" "${UTEST_FOLDER}/file4.txt" ) 
else
  FILE_LIST=("${UTEST_FOLDER}/file1.txt" "${UTEST_FOLDER}/file2.txt" "${UTEST_FOLDER}/file3.txt" "${UTEST_FOLDER}/file4.txt" ) 
fi

PATTERNS=${UTEST_FOLDER}/all_patterns.txt
for FILENAME in ${FILE_LIST[@]}
do
  rm -f ${FILENAME}.lz*  
  echo ""
  echo "**************************"
  echo "Testing on ${FILENAME}"
  echo "**************************"
  echo ""
  ./test_save_load.sh ${FILENAME} ${PATTERNS}
done

if [[ ${HOST} =~ hp8x-37 || ${HOST} =~ whq ]]; then
  echo "Done in the local machine..."
  #FILENAME=${UTEST_FOLDER}/ABCD_text.txt
  #SHORT_PATTERNS=${UTEST_FOLDER}/ABCD_patterns.txt
  #./test_save_load.sh ${FILENAME} ${SHORT_PATTERNS}
else
  # just one more...
  FILENAME=${UTEST_FOLDER}/ABCD_text.txt
  SHORT_PATTERNS=${UTEST_FOLDER}/ABCD_patterns.txt
  ./test_save_load.sh ${FILENAME} ${SHORT_PATTERNS}
fi

utils_success_exit
