
#!/usr/bin/env bash
set -o errexit
set -o nounset
set -e 
set -o pipefail

PAN_REFERENCE_FOLDER=$1
SAM_FOLDER=$2
SENSIBILITY=4
SAM_TO_POS_PY=../components/sam_to_pos/scripts/sam_to_positions.py
#GAP_POS_PY=../components/sam_to_pos/scripts/gap_positions.py
N=30

#DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
#source ${DIR}/config.sh

echo "Pangenome reference folder is: ${PAN_REFERENCE_FOLDER}"
echo "Sam folder is: ${SAM_FOLDER}"

shopt -s nullglob dotglob

export SHELL=$(type -p bash)

#Filter only mapped reads
task() {
  CURRENT_REFERENCE=$1
  SAM_FOLDER=$2
  # Make the calls
  CUR=$(basename -- "$CURRENT_REFERENCE")
  #CUR="${CUR_NAME%.*}"
  SAM_FILE=${SAM_FOLDER}/${CUR}.sam.gz
  #Filter mapped reads
  echo " grep -h ${CUR} | gzip > ${SAM_FILE}"
  grep -h ${CUR} $SAM_FOLDER/*.sam > ${SAM_FILE}
}

start=`date +%s`

export -f task

echo "Splitting into many SAM FILES:"
ls ${PAN_REFERENCE_FOLDER}* | parallel -j$N task {} $SAM_FOLDER

#sleep 5

echo "SAM files generated"

#N_REFS=$( ls ${PAN_REFERENCE_FOLDER} | wc -l )

