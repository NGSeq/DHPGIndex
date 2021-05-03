#!/usr/bin/env bash

set -e
#set -o pipefail
#set -v
#set -x

export SPARK_MAJOR_VERSION=2

LOCALINDEXPATH=/mnt/tmp
HDFSURI=hdfs://node-1:8020 # HDFS URI
NODE=node- #Basename for nodes in the cluster. The nodes must be numbered starting from 1.
MAX_QUERY_LEN=1000 # Set maximum sequence length for index queries (should be > max read length or > BLAST alignment length)
HDFSUSERDIR=$HDFSURI/user/root
PGPATHLOCAL=$1 #directory containing pan-genome files in FASTA format
PGPATHHDFS=$HDFSUSERDIR/pg
DRLZPATHHDFS=$HDFSUSERDIR/drlz
SPARKMASTER=yarn-client #For single node testing use local[n] where n is the amount of executors,
N=26 #number of cluster nodes

#Use paired-end reads with Bowtie etc. read aligners.
#READS_1=$2
#READS_2=$3

#Use with BLAST
QSEQS=$2 #path to single file in local fs
hdfs dfs -mkdir -p $HDFSUSERDIR/qseqs
hdfs dfs -put $QSEQS $HDFSUSERDIR/qseqs/

hdfs dfs -mkdir -p $PGPATHHDFS

date >> runtime.log
echo "Loading files to HDFS..."

#seq 1 $N | xargs -I{} -n 1 -P $N hdfs dfs -put $PGPATHLOCAL/{} $PGPATHHDFS
hdfs dfs -put $PGPATHLOCAL/* $PGPATHHDFS

date >> runtime.log
echo "Starting DRLZ.." 
start=`date +%s`
#seq 1 $N | xargs -I{} -n 1 -P $N ./drlz_microbes.sh {} pg lz groupedfasta 0.33 &
./drlz_microbes.sh $PGPATHHDFS $DRLZPATHHDFS $HDFSUSERDIR/groupedfasta 0.33 $HDFSURI $SPARKMASTER
runtime=$((end-start))
echo "DRLZ compression time: ${runtime}" >> runtime.log

date >> runtime.log
echo "Starting distributed indexing.."
start=`date +%s`

hdfs dfs -mkdir $HDFSUSERDIR/blasted
QFNAME=$(basename -- "$QSEQS")
./distributed_indexing.sh $HDFSUSERDIR/groupedfasta $DRLZPATHHDFS $LOCALINDEXPATH $MAX_QUERY_LEN $HDFSURI $SPARKMASTER $HDFSUSERDIR/qseqs/$QFNAME

#Using single merged index (useful with Bowtie. "makeblastdb" is limited to 4GB input)

#echo "Merging indexes.."
#start=`date +%s`
#mkdir -p $LOCALINDEXPATH/merged/
#seq 1 $N | xargs -I{} -n 1 -P $N scp -o "StrictHostKeyChecking no" $NODE{}:$LOCALINDEXPATH/*.fa.* $LOCALINDEXPATH/merged/

#./merge_blast_index.sh $LOCALINDEXPATH/merged $N

#/opt/dhpgindex/chic_index --threads=16  --kernel=BOWTIE2 --verbose=2 --indexing --lz-input-file=$LOCALINDEXPATH/merged/merged_phrases.lz $LOCALINDEXPATH/merged/ ${MAX_QUERY_LEN}

end=`date +%s`
runtime=$((end-start))
echo "Indexed and aligned in: ${runtime}" >> runtime.log
date >> runtime.log

hdfs dfs -get $HDFSUSERDIR/blasted
echo "Alignments downloaded from HDFS"


#echo "Aligning sequences.."
#start=`date +%s`
#Align to single index
#/opt/dhpgindex/chic_align -v1 -t 16 -o /mnt/tmp/aligned $LOCALINDEXPATH/merged/merged $READS_1 $READS_2 #with Bowtie an paired-end reads use $READS_1 $READS_2
#/opt/samtools/samtools view -F4 /mnt/tmp/aligned.sam > /mnt/tmp/mapped.sam
#end=`date +%s`
#runtime=$((end-start))
#echo "Aligned sequences in: ${runtime}" >> runtime.log



                                                 
