#!/usr/bin/env bash
set -o errexit
set -o nounset

source "./utils.sh"
DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
if [ "$#" -ne 0 ]; then
  echo "Script: '${0}'"
  echo "uses no params instead of $# " 
  exit
fi

cd ..;


FLAGS='--vgdb-error=1 --leak-check=full --show-reachable=yes'
#PREPROCESSOR="valgrind ${FLAGS}"
#PREPROCESSOR="gdb --args"
PREPROCESSOR=""

M=3
FILE="${DIR}/utest_data/file1.txt"


for MAX_K in 0 5
do
  rm -f ${FILE}.*
  ${PREPROCESSOR} ./src/build_index --verbose=${VERBOSE_LEVEL} --kernel=FMI --max-edit-distance=${MAX_K} ${FILE} ${M}

  KERNEL="${FILE}.P512_GC4_kernel_text"
  EXPECTED_KERNEL="${DIR}/utest_data/expected/file1.M3.K${MAX_K}.kernel"

  utils_assert_equal_files ${KERNEL} ${EXPECTED_KERNEL}
  rm -f ${FILE}.*
done

cd ${DIR}
