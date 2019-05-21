#!/usr/bin/env bash
set -o errexit
set -o nounset

source "./utils.sh"
DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
if [ "$#" -ne 2 ]; then
  echo "Script: '${0}'"
  echo "uses 2 params instead of $# " 
  exit
fi

utils_assert_file_exists ${1} 
utils_assert_file_exists ${2} 


FILE=`readlink -f ${1}`
PATTERNS=`readlink -f ${2}`

cd ..;

FLAGS='--vgdb-error=1 --leak-check=full --show-reachable=yes'
#PREPROCESSOR="valgrind ${FLAGS}"
#PREPROCESSOR="gdb --args"
PREPROCESSOR=""
M_SIZES=( 10 )
K_SIZES=( 0 2 4 )

HOST="$(hostname)"
if [[ ${HOST} =~ hp8x-37 || ${HOST} =~ whq ]]; then
  echo "Local machine..."
  #M_SIZES=( 10 30 50 )
else
  #CI Setting, more demanding.
  #PREPROCESSOR="valgrind ${FLAGS}" ## TODO: we need to remove this to have stricter test in CI
  M_SIZES=( 10 30 50 )
  K_SIZES=( 0 2 4 10 )
fi


INDEX_BASENAME=./different_index_prefix
N_SETTINGS=3

for MAX_K in ${K_SIZES[@]}
do
  for M in ${M_SIZES[@]}
  do
    #In Memory
    FLAGS_BUILD[0]="--max-edit-distance=${MAX_K} --kernel=FMI ${FILE} ${M}"
    FLAGS_LOAD[0]="--validation_test ${FILE} ${PATTERNS}"

    #External memory
    FLAGS_BUILD[1]="--verbose=${VERBOSE_LEVEL} --max-edit-distance=${MAX_K} --kernel=FMI --lz-parsing-method=EM ${FILE} ${M}"
    FLAGS_LOAD[1]="--validation_test ${FILE} ${PATTERNS}"

    #Different basename for output. No validation test, because it would  need to know the original input file.
    FLAGS_BUILD[2]="--max-edit-distance=${MAX_K} --kernel=FMI --output=${INDEX_BASENAME} --lz-parsing-method=EM ${FILE} ${M}"
    FLAGS_LOAD[2]="${INDEX_BASENAME} ${PATTERNS}"
    
    # RLX
    FLAGS_BUILD[3]="--verbose=${VERBOSE_LEVEL} --max-edit-distance=${MAX_K} --kernel=FMI --lz-parsing-method=RLZ ${FILE} ${M}"
    FLAGS_LOAD[3]="--validation_test ${FILE} ${PATTERNS}"
    
    # LZ AS A PARAM. Requires previous run...
    FLAGS_BUILD[4]="--verbose=${VERBOSE_LEVEL} --max-edit-distance=${MAX_K} --kernel=FMI --lz-input-file=PREV_LZ_PARSE.lzparse ${FILE} ${M}"
    FLAGS_LOAD[4]="--validation_test ${FILE} ${PATTERNS}"
    #for SETTING_I in 0 1 2
    for SETTING_I in 0 1 2 3 4
    #for SETTING_I in 3
    do
      rm -f ${FILE}.*
      rm -f ${INDEX_BASENAME}.*
      BUILD_ARGS="--verbose=${VERBOSE_LEVEL} ${FLAGS_BUILD[${SETTING_I}]}"
      LOAD_ARGS="--verbose=${VERBOSE_LEVEL} ${FLAGS_LOAD[${SETTING_I}]}"
      ############################################
      ${PREPROCESSOR} ./src/build_index ${BUILD_ARGS}
      utils_pause_with_timeout 'Press any key to proceed with LOAD test...'
      ${PREPROCESSOR} ./src/load_index ${LOAD_ARGS}
      utils_pause_with_timeout 'Done.'
      # If setting _i eq 3  ?....
      if [[ ${SETTING_I} -eq 3 ]]; then
        ls ${FILE}*.lzparse 
        cp ${FILE}*.lzparse PREV_LZ_PARSE.lzparse
      fi

    done
  done
done
rm -f ${FILE}.*
rm -f ${INDEX_BASENAME}.*

cd ${DIR}
