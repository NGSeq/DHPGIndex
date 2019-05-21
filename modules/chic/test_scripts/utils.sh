#!/usr/bin/env bash
set -o errexit
set -o nounset
set -o pipefail

source "./global_config.sh"

# Adaptation of Martin Burger's bash common helpers.
# https://github.com/martinburger/bash-common-helpers/blob/master/bash-common-helpers.sh
#
#
# Copyright (c) 2014 Martin Burger
# Released under the MIT License (MIT)
# https://github.com/martinburger/bash-common-helpers/blob/master/LICENSE

function utils_assert_equal_files
{
  local fileA=${1}
  local fileB=${2}
  utils_assert_file_exists ${fileA}
  utils_assert_file_exists ${fileB}
  diff ${fileA} ${fileB}
  #vimdiff ${fileA} ${fileB}
}

function utils_success_exit
{
  HOST="$(hostname)"
  TALK=""
  if [[ ${HOST} =~ hp8x-37 || ${HOST} =~ whq ]]; then
    TALK=1
  else
    TALK=0
  fi
  
  red=""
  reset=""
  ## Put here any other message I want to deliver on exit...
  if [[ ${TALK} -eq 1 ]]; then
    spd-say "I'm ready"
    local red=$(tput setaf 2)
    local reset=$(tput sgr0)
  fi
  echo >&2 -e "${red}Success!${reset}"
  exit 0
}

function utils_die 
{
  local red=$(tput setaf 1)
  local reset=$(tput sgr0)
  echo >&2 -e "${red}$@${reset}"
  exit 1
}

function utils_assert_file_exists 
{
  local file=${1}
  if [[ ! -f "${file}" ]]; then
    utils_die "Cancelling because required file '${file}' does not exist."
  fi
}

function utils_assert_list_of_files_exists ()
{
  for FILE in $@
  do
    utils_assert_file_exists ${FILE}
    #echo "V..."
  done
}

function utils_assert_file_does_not_exist 
{
  local file=${1}
  if [[ -e "${file}" ]]; then
    utils_die "Cancelling because file '${file}' exists."
  fi
}

function utils_pause()
{
local msg=${1}
read -p "${msg} (click something to resume) " -n 1 dummy_answer || dummy_answer=K
}

function utils_pause_with_timeout()
{
local msg=${1}
read -t ${GLOBAL_TIMEOUT} -p "${msg} (click something to speed up, otherwise ressuming soon) " -n 1 dummy_answer || dummy_answer=K
}



function utils_ask_to_continue 
{
  local msg=${1}
  local waitingforanswer=true
  while ${waitingforanswer}; do
    read -p "${msg} (hit 'y/Y' to continue, 'n/N' to cancel) " -n 1 ynanswer
    case ${ynanswer} in
      [Yy] ) waitingforanswer=false; break;;
      [Nn] ) echo ""; utils_die "Operation cancelled as requested!";;
      *    ) echo ""; echo "Please answer either yes (y/Y) or no (n/N).";;
    esac
    done
    echo ""
}

function utils_ask_to_run_command
{
  local comm=${1}
  local waitingforanswer=true
  while ${waitingforanswer}; do
    echo "Would you like to run the following command (y/n) ? (or just wait 10 seconds to continue)"
    echo ${comm}
    read -t ${GLOBAL_TIMEOUT} -p "(hit 'y/Y' to run it, 'n/N' or wait ${GLOBAL_TIMEOUT} secs to continue) " -n 1 ynanswer || ynanswer=N
    case ${ynanswer} in
      [Yy] ) waitingforanswer=false; ${comm}; break;;
      [Nn] ) waitingforanswer=false; echo ""; echo "Command skipped as requested!";;
      *    ) echo ""; echo "Please answer either yes (y/Y) or no (n/N) or wait.";;
    esac
    done
    echo ""
}
