#!/usr/bin/env bash
set -o errexit
set -o nounset
set -o pipefail 

source "./utils.sh"

# about 200 M
#INPUT_FILE=/home/dvalenzu/PHD/BIO/hybrid_index/test_scripts/bio_data/genome
INPUT_FILE=/home/dvalenzu/PHD/repos_corpus/repetitive_corpus/coreutils/coreutils
#INPUT_FILE=/home/dvalenzu/PHD/repos_corpus/repetitive_corpus/einstein/einstein.en.txt
## 10 M

# NEEDS TO BE AS LARGE AS THE INPUT FILE.
# OTHERWISE, IT IS VERY LIKELY TO CRASH.
DECODE_BUFFER_SIZE=800
## The decode buffer needs to be sligthly larger than the ref.

#MAX_MEM_MB=100
MAX_MEM_MB=1000
#MAX_MEM_MB=10000


CODER_BIN=./emlz_parser
DECODER_BIN=../../LZ-Decoder/decode
utils_assert_file_exists ${CODER_BIN} 
utils_assert_file_exists ${DECODER_BIN} 

COMPRESSED_FILE=tmp.rlz_compressed
DECOMPRESSED_FILE=tmp.reconstructed
rm -f ${COMPRESSED_FILE}
rm -f ${DECOMPRESSED_FILE}

echo "Encoding..."
${CODER_BIN} ${INPUT_FILE} --mem=${MAX_MEM_MB} --output=${COMPRESSED_FILE}
echo "Decoding..."
${DECODER_BIN}  ${COMPRESSED_FILE} ${DECOMPRESSED_FILE} ${DECODE_BUFFER_SIZE}
echo "Verifying (diff)..."
diff ${DECOMPRESSED_FILE} ${INPUT_FILE}

echo "Test succesfull"
