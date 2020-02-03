
#!/usr/bin/env bash
set -o errexit
set -o nounset
set -e 
set -o pipefail

PAN_REFERENCE_FOLDER=$1
SAM_FOLDER=$2
SENSIBILITY=4
SAM_TO_POS_PY=../components/sam_to_pos/scripts/sam_to_positions.py
N=30

#DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
#source ${DIR}/config.sh

echo "Pangenome reference folder is: ${PAN_REFERENCE_FOLDER}"
echo "Sam folder is: ${SAM_FOLDER}"

shopt -s nullglob dotglob

export SHELL=$(type -p bash)

#N_REFS=$( ls ${PAN_REFERENCE_FOLDER} | wc -l )

task2() {
  
  CURRENT_REFERENCE_FULL=$1
  BASE=$(basename -- "$CURRENT_REFERENCE_FULL")
  SAM=$2/${BASE}.sam.gz
  POSITIONS_FILE=$2/${BASE}.pos

  echo "sam_to_positions.py $SAM_TO_POS_PY $SAM $CURRENT_REFERENCE_FULL $SENSIBILITY"
  python $SAM_TO_POS_PY $SAM $CURRENT_REFERENCE_FULL $SENSIBILITY > $POSITIONS_FILE 	
  
  #GAP_POS_FILE=$2/${BASE}.gap_positions
  #python $GAP_POS_PY $CURRENT_REFERENCE_FULL > $GAP_POS_FILE

}

export -f task2

ls ${PAN_REFERENCE_FOLDER}* | parallel -j$N task2 {} $SAM_FOLDER
#for i in ${PAN_REFERENCE_FOLDER}*
#do
# task2 $i $SAM_FOLDER & 
#done

end=`date +%s`
#hdfs dfs -put ${SAM_FOLDER}/*.pos pos
runtime=$((end-start))
echo "pos: ${runtime}" >> sams.log
