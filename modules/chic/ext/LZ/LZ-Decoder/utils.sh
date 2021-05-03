#!/usr/bin/env bash
set -o errexit
set -o nounset
set -o pipefail

# Adaptation of Martin Burger's bash common helpers.
# https://github.com/martinburger/bash-common-helpers/blob/master/bash-common-helpers.sh
#
#
# Copyright (c) 2014 Martin Burger
# Released under the MIT License (MIT)
# https://github.com/martinburger/bash-common-helpers/blob/master/LICENSE

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

function utils_assert_file_does_not_exist 
{
  local file=${1}
  if [[ -e "${file}" ]]; then
    utils_die "Cancelling because file '${file}' exists."
  fi
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
