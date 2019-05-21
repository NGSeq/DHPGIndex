#!/usr/bin/env bash
set -o errexit
set -o nounset
set -o pipefail 

DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
source "${DIR}/utils.sh"
  
main() 
{
if [ "$#" -ne 4 ]; then
    echo "Script: '${0}'"
    echo "Function name:  ${FUNCNAME}"
    echo "uses 4 params instead of $#" 
    echo "${0} INPUT_FILE MAX_MEM_MB REF_SIZE_MB OUTPUT_FILE "
    utils_die 
 fi

INPUT_FILE=${1}
MAX_MEM_MB=${2}
REF_SIZE_MB=${3}
OUT_FILE=${4}
N_CHUNKS=1
N_THREADS=1
#" A LOT" so that the whole thing fits memory...
# 12 GB for the local machines
# Still to get "THE" RLZ I shouldn use this

MEASURE_TOOL="valgrind --vgdb-error=1 --leak-check=full --show-leak-kinds=all"

CODER_BIN="${DIR}/../RLZ_parallel/src/rlz_for_hybrid"

utils_assert_file_exists ${INPUT_FILE}
utils_assert_file_exists ${CODER_BIN}

REF_FILE=tmp.reference
echo "Doing Prefix-ref ..."
REF_SIZE_BYTES=$(( ${REF_SIZE_MB}*1024*1024 ))

echo "head -c ${REF_SIZE_BYTES} ${INPUT_FILE} > ${REF_FILE}"
head -c ${REF_SIZE_BYTES} ${INPUT_FILE} > ${REF_FILE}
echo "Tmp ref file created..."

echo "Prefix-ref created with head..."
echo "Encoding..."
time  ${CODER_BIN} ${REF_FILE} ${INPUT_FILE} ${OUT_FILE} ${N_CHUNKS} ${N_THREADS} ${MAX_MEM_MB}
#${MEASURE_TOOL}  ${CODER_BIN} ${REF_FILE} ${INPUT_FILE} ${OUT_FILE} ${N_CHUNKS} ${N_THREADS} ${MAX_MEM_MB}
echo "Parser script finished."
}

main "$@"
