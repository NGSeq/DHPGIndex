#!/usr/bin/env bash
set -o errexit
set -o nounset
set -o pipefail 

CURR_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
source "./utils.sh"

MEASURE_TOOL=""
#MEASURE_TOOL="gdb --args"
#MEASURE_TOOL="valgrind --vgdb-error=1 --leak-check=full --show-leak-kinds=all"

# about 200 M
#INPUT_FILE=/home/dvalenzu/PHD/BIO/hybrid_index/test_scripts/bio_data/genome
#INPUT_FILE=/home/dvalenzu/PHD/repos_corpus/repetitive_corpus/coreutils/coreutils
#INPUT_FILE=/home/dvalenzu/PHD/repos_corpus/repetitive_corpus/einstein/einstein.en.txt
#INPUT_FILE="${CURR_DIR}/example_6G"
INPUT_FILE="${CURR_DIR}/../../../../test_scripts/utest_data/ABCD_text.txt"
#INPUT_FILE="/home/scratch-hdd/dvalenzu/data/PG_PREPROCESSOR_DATA/all_fastas/CHR1/samp5G.fasta"
## 10 M

# NEEDS TO BE AS LARGE AS THE INPUT FILE.
# OTHERWISE, IT IS VERY LIKELY TO CRASH.
DECODE_BUFFER_SIZE=8000
## The decode buffer needs to be sligthly larger than the ref.

#MAX_MEM_MB=100
MAX_MEM_MB=1000
#MAX_MEM_MB=10000


CODER_BIN=./compressKKP3
DECODER_BIN=../../LZ-Decoder/decode
utils_assert_file_exists ${CODER_BIN} 
utils_assert_file_exists ${DECODER_BIN} 

COMPRESSED_FILE=tmp.rlz_compressed
DECOMPRESSED_FILE=tmp.reconstructed
rm -f ${COMPRESSED_FILE}
rm -f ${DECOMPRESSED_FILE}

echo "Encoding..."
${MEASURE_TOOL} ${CODER_BIN} ${INPUT_FILE} ${COMPRESSED_FILE}
echo "Decoding..."
${DECODER_BIN}  ${COMPRESSED_FILE} ${DECOMPRESSED_FILE} ${DECODE_BUFFER_SIZE}
echo "Verifying (diff)..."
diff ${DECOMPRESSED_FILE} ${INPUT_FILE}

echo "Test succesfull"
