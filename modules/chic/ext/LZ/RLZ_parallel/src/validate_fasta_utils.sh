#!/usr/bin/env bash
set -o errexit
set -o nounset
set -o pipefail 

source "./utils.sh"

function validate_fasta
{
  INPUT_FILE=${1}
  #N_CHUNKS=10
  N_CHUNKS=20  
  # ch=30 will test feature to allow chunks shorter than ref.
  N_THREADS=4

  ## TODO: run it on all 5,6 ,7, 8 
  # about 200 M
  #INPUT_FILE=/home/dvalenzu/PHD/repos_corpus/repetitive_corpus/coreutils/coreutils
  #INPUT_FILE=/home/dvalenzu/PHD/repos_corpus/repetitive_corpus/einstein/einstein.en.txt
  ## 10 M


  MAX_MEM_MB=100
  # NEEDS TO BE AS LARGE AS THE INPUT FILE.
  # OTHERWISE, IT IS VERY LIKELY TO CRASH.
  DECODE_BUFFER_SIZE=200
  ## The decode buffer needs to be sligthly larger than the ref.

  REF_SIZE_MB=0
  #REF_SIZE_MB=10

  CODER_BIN=./rlz_parser.sh
  DECODER_BIN=../../LZ-Decoder/decode
  utils_assert_file_exists ${CODER_BIN} 
  utils_assert_file_exists ${DECODER_BIN} 

  COMPRESSED_FILE=tmp.rlz_compressed
  DECOMPRESSED_FILE=tmp.reconstructed
  rm -f ${COMPRESSED_FILE}
  rm -f ${DECOMPRESSED_FILE}

  echo "Encoding..."
  ${CODER_BIN} ${INPUT_FILE} ${COMPRESSED_FILE} ${REF_SIZE_MB} ${N_CHUNKS} ${N_THREADS} ${MAX_MEM_MB}
  echo "Decoding..."
  ${DECODER_BIN}  ${COMPRESSED_FILE} ${DECOMPRESSED_FILE} ${DECODE_BUFFER_SIZE}
  echo "Verifying (diff)..."
  #cmp ${DECOMPRESSED_FILE} ${INPUT_FILE}

  TMP_FILE=tmp.output
  utils_fasta_to_plain ${INPUT_FILE} ${TMP_FILE}
  #vimdiff ${DECOMPRESSED_FILE} ${TMP_FILE}
  cmp ${DECOMPRESSED_FILE} ${TMP_FILE}
  #echo "vimdiff ${DECOMPRESSED_FILE} ${TMP_FILE}"
}

