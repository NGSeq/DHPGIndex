#!/usr/bin/env bash
set -o errexit
set -o nounset
set -o pipefail 

source "./utils.sh"
#N_CHUNKS=10
N_CHUNKS=20  
# ch=30 will test feature to allow chunks shorter than ref.
N_THREADS=4


# about 200 M
INPUT_FILE=/home/dvalenzu/PHD/repos_corpus/repetitive_corpus/coreutils/coreutils
#INPUT_FILE=/home/dvalenzu/PHD/repos_corpus/repetitive_corpus/einstein/einstein.en.txt
## 10 M


MAX_MEM_MB=100
# NEEDS TO BE AS LARGE AS THE INPUT FILE.
# OTHERWISE, IT IS VERY LIKELY TO CRASH.
DECODE_BUFFER_SIZE=200
## The decode buffer needs to be sligthly larger than the ref.

REF_SIZE_MB=10

CODER_BIN=./rlz_sequential_parser.sh
DECODER_BIN=../LZ-Decoder/decode
utils_assert_file_exists ${CODER_BIN} 
utils_assert_file_exists ${DECODER_BIN} 

COMPRESSED_FILE=tmp.rlz_compressed
DECOMPRESSED_FILE=tmp.reconstructed
rm -f ${COMPRESSED_FILE}
rm -f ${DECOMPRESSED_FILE}

echo "Encoding..."
${CODER_BIN} ${INPUT_FILE} ${MAX_MEM_MB} ${REF_SIZE_MB} ${COMPRESSED_FILE}
echo "Decoding..."
${DECODER_BIN}  ${COMPRESSED_FILE} ${DECOMPRESSED_FILE} ${DECODE_BUFFER_SIZE}
echo "Verifying (diff)..."
cmp ${DECOMPRESSED_FILE} ${INPUT_FILE}

utils_success_exit
