#!/usr/bin/env bash
set -o errexit
set -o nounset
set -o pipefail

source "./utils.sh"
DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

MAX_ED=10
#MAX_ED=1
GRAL_BUILD_FLAGS=" --max-edit-distance=${MAX_ED}"


./clean.sh
for METHOD in "IM" "RLZ"
do
  for FOLDER in "${DIR}/bio_data/larger/set_1"
  do
    REFERENCE=${FOLDER}/genome.fa
    READS=${FOLDER}/reads.fq
    EXPECTED_MATCHES=${FOLDER}/expected_matches
    rm -f ${REFERENCE}.*  
    echo "**************************"
    echo "Testing on ${REFERENCE}"
    echo "**************************"

    for KERNEL in BWA BOWTIE2
    do
      echo "TESTING KERNEL: ${KERNEL}"
      OTHER_BUILD_FLAGS="${GRAL_BUILD_FLAGS} --kernel=${KERNEL} --lz-parsing-method=${METHOD}"
      if [[${METHOD} == "RLZ"]]; then
        OTHER_BUILD_FLAGS="${OTHER_BUILD_FLAGS} --rlz-ref-size=0"
      fi

      ./test_bio_detail.sh ${REFERENCE} ${READS} "${OTHER_BUILD_FLAGS}" ${EXPECTED_MATCHES}
    done
  done
done
./clean.sh

utils_success_exit
