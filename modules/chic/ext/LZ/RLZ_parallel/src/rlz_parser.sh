#!/usr/bin/env bash
set -o errexit
set -o nounset
set -o pipefail 

DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
source "${DIR}/utils.sh"
  
main() 
{
if [ "$#" -ne 6 ]; then
    echo "Script: '${0}'"
    echo "Function name:  ${FUNCNAME}"
    echo "uses 6 params instead of $#" 
    echo "${0} INPUT_FILE OUTPUT_FILE REF_SIZE_MB N_CHUNKS N_THREADS MAX_MEM_MB"
    utils_die 
 fi

INPUT_FILE=${1}
OUT_FILE=${2}
REF_SIZE_MB=${3}
N_CHUNKS=${4}
N_THREADS=${5}
MAX_MEM_MB=${6}

FILENAME=$(basename "${1}")
EXTENSION="${FILENAME##*.}"
echo "Extension: ${EXTENSION}"

FASTA_FLAG=0
if [[ "${EXTENSION}" = "fasta" ]]; then
  echo "fasta... setting flag"
  FASTA_FLAG=1
fi
if [[ "${EXTENSION}" = "fa" ]]; then
  echo "fa... setting flag"
  FASTA_FLAG=1
fi


# When dbugging...
MEASURE_TOOL=""
#MEASURE_TOOL="gdb --args"
#MEASURE_TOOL="valgrind --vgdb-error=1 --leak-check=full --show-leak-kinds=all"

CODER_BIN="${DIR}/rlz_for_hybrid"

utils_assert_file_exists ${INPUT_FILE}
utils_assert_file_exists ${CODER_BIN}

echo "Encoding..."
#time  ${CODER_BIN} ${INPUT_FILE} ${REF_SIZE_MB} ${INPUT_FILE} ${OUT_FILE} ${N_CHUNKS} ${N_THREADS} ${MAX_MEM_MB} ${FASTA_FLAG}
${MEASURE_TOOL}  ${CODER_BIN} ${INPUT_FILE} ${REF_SIZE_MB} ${INPUT_FILE} ${OUT_FILE} ${N_CHUNKS} ${N_THREADS} ${MAX_MEM_MB} ${FASTA_FLAG}
echo "Parser script finished."
}

main "$@"
