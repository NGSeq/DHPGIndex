#!/usr/bin/env bash
set -o errexit
set -o nounset
set -o pipefail 

rm -f filtered_input*
rm -f HI_*
rm -f Reads_*
rm -f tmp*
rm -f .tmp*
rm -f .reads*
rm -f ./bio_data/data_2/genome.fa.*
rm -f ./bio_data/data_3/genome.fa.*
rm -rf ./TMP*
for FOLDER in ./bio_data/data*
do
  rm -f ${FOLDER}/genome.fa.*
done

for FOLDER in ./bio_data/larger/set_*
do
  rm -f ${FOLDER}/genome.fa.*
done
