#!/usr/bin/env bash
set -o errexit
set -o nounset
set -o pipefail

function bio_count_unique_reads_sam 
{
  local FILE_NAME=${1}
  ANS=`cat ${FILE_NAME} | grep -v "@" | sed "s/\t.*//" | sort | uniq | wc -l`
  echo ${ANS}
}

function bio_count_unique_reads_fq 
{
  local FILE_NAME=${1}
  ANS=`cat ${FILE_NAME} | grep  "@" | sort | uniq | wc -l`
  echo ${ANS}
}

function bio_validate_outputSAM_inputFQ 
{
  local SAM_NAME=${1}
  local FQ_NAME=${2}
  utils_assert_file_exists ${SAM_NAME}
  utils_assert_file_exists ${FQ_NAME}
  N_SAM=$( bio_count_unique_reads_sam ${SAM_NAME} )
  N_FQ=$( bio_count_unique_reads_fq ${FQ_NAME} )
  echo "SAM: ${N_SAM}, FQ: ${N_FQ}"
  if [[ ${N_SAM} -eq ${N_FQ} ]]; then
    return 0
  else
    echo "********************"
    echo "********************"
    echo "********************"
    echo "Different number of reads, will abort, and offer you to check."
    echo "********************"
    utils_ask_to_run_command "vim -O ${SAM_NAME} ${FQ_NAME}"
    utils_die "Lost read :("
  fi
}

function bio_validate_expected_matches
{
  local SAM_NAME=${1}
  local EXPECTED_MATCHES=${2}
  utils_assert_file_exists ${SAM_NAME}
  utils_assert_file_exists ${EXPECTED_MATCHES}

  cat ${EXPECTED_MATCHES} | while read LINE
  do
    COMMAND="grep $'${LINE}' ${SAM_NAME} | wc -l"
    #echo "Command: '${COMMAND}'"
    OCCS=$(echo ${COMMAND} | bash)
    if [ "${OCCS}" -eq "0" ]; then 
      utils_die "No occurrence of expected match! Check '${SAM_NAME}' out!"
    else
      echo ":)"
    fi
  done
}
