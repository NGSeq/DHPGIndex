#!/usr/bin/env bash
set -o errexit
set -o nounset
set -o pipefail 

source "./utils.sh"
source "./validate_fasta_utils.sh"

DATA_DIR="../../../../test_scripts/bio_data/"
LIST="data_2 data_3 data_4 data_5 data_6 data_7 data_8"
for FOLDER_NAME in ${LIST}; do
  INPUT_FILE="${DATA_DIR}/${FOLDER_NAME}/genome.fa"
  validate_fasta ${INPUT_FILE}  > test_${FOLDER_NAME}.log
done
utils_success_exit
