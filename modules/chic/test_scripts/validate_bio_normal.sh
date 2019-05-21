#!/usr/bin/env bash
set -o errexit
set -o nounset
set -o pipefail

source "./utils.sh"
DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

## TODO: this is not working well if I want to put more than one param.
#OTHER_BUILD_FLAGS=" --max-edit-distance=10"
OTHER_BUILD_FLAGS=" --max-edit-distance=1"

./clean.sh

for FOLDER in "${DIR}/bio_data/data_2/" "${DIR}/bio_data/data_3/" "${DIR}/bio_data/data_4/"
#for FOLDER in "${DIR}/bio_data/data_4/"
do
  REFERENCE=${FOLDER}/genome.fa
  READS=${FOLDER}/reads.fq
  rm -f ${REFERENCE}.*  
  echo "**************************"
  echo "Testing on ${REFERENCE}"
  echo "**************************"

  #for KERNEL in BOWTIE2 BWA
  for KERNEL in BOWTIE2
  do
    echo "TESTING KERNEL: ${KERNEL}"
    ./test_bio_im.sh ${REFERENCE} ${READS} ${KERNEL} ${OTHER_BUILD_FLAGS}
    #######./test_bio_em.sh ${REFERENCE} ${READS} ${KERNEL} ${OTHER_BUILD_FLAGS}
    ./test_bio_all.sh ${REFERENCE} ${READS} ${KERNEL} ${OTHER_BUILD_FLAGS}
    ./test_bio_lzparam.sh ${REFERENCE} ${READS} ${KERNEL} ${OTHER_BUILD_FLAGS}
  done
  ./test_bio_compare_kernels.sh ${REFERENCE} ${READS} ${OTHER_BUILD_FLAGS}
done
./clean.sh

utils_success_exit
