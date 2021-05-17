#!/usr/bin/env bash

set -e
#set -o pipefail
#set -v
#set -x

export SPARK_MAJOR_VERSION=2

PGPATHLOCAL=$1 #Local directory containing pan-genome files in FASTA format
#Use with BLAST
QSEQS=$2 #Local path to single FASTA file (query file)
#Use paired-end reads with Bowtie etc. read aligners.
#READS_1=$2
#READS_2=$3

N=1 #Number of cluster nodes, N=1 is just for testing, HDFS requires at least 3 datanodes for replication as default
LOCALINDEXPATH=/mnt/tmp #Intermediate files and the final index are stored here
HDFSURI=hdfs://node-1:8020 # HDFS URI
NODE=node- #Basename for nodes in the cluster. The nodes must be numbered starting from 1.
MAX_QUERY_LEN=1000 # Set maximum sequence length for index queries (should be > max read length or > BLAST alignment length)
DICTSIZE=0.33 #Fraction of dictionary in decimals (maximum is 1.0)
HDFSUSERDIR=$HDFSURI/user/root
PGPATHHDFS=$HDFSUSERDIR/pg #HDFS path to store pan-genome
DRLZPATHHDFS=$HDFSUSERDIR/drlz #HDFS path to store compressed data

SPARKMASTER=local[4] #Recommended: "yarn-client" or "yarn-cluster". For single node testing use local[n] where n is the amount of executors.
EXECUTORMEM=6g  #Spark executor memory. If YARN used, cannot exceed yarn.nodemanager.resource.memory-mb defined in /opt/hadoop/etc/hadoop/yarn-site.xml
DRIVERMEM=6g  #Spark driver memory
THREADS=2 # Number of threads used with chic indexing and alignment (passed to Bowtie and BLAST parameters). Should not exceed the total number of cores of the node.

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
./drlz_microbes.sh $PGPATHHDFS $DRLZPATHHDFS $HDFSUSERDIR/groupedfasta $DICTSIZE $HDFSURI $SPARKMASTER $DRIVERMEM $EXECUTORMEM
runtime=$((end-start))
echo "DRLZ compression time: ${runtime}" >> runtime.log

date >> runtime.log
echo "Starting distributed indexing and alignment.."
start=`date +%s`

hdfs dfs -mkdir $HDFSUSERDIR/blasted
QFNAME=$(basename -- "$QSEQS")
./distributed_indexing.sh $HDFSUSERDIR/groupedfasta $DRLZPATHHDFS $LOCALINDEXPATH $MAX_QUERY_LEN $HDFSURI $SPARKMASTER $HDFSUSERDIR/qseqs/$QFNAME $THREADS

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
echo "Alignments downloaded from HDFS to folder blasted"


#echo "Aligning sequences.."
#start=`date +%s`
#Align to single index
#/opt/dhpgindex/chic_align -v1 -t 16 -o /mnt/tmp/aligned $LOCALINDEXPATH/merged/merged $READS_1 $READS_2 #with Bowtie an paired-end reads use $READS_1 $READS_2
#/opt/samtools/samtools view -F4 /mnt/tmp/aligned.sam > /mnt/tmp/mapped.sam
#end=`date +%s`
#runtime=$((end-start))
#echo "Aligned sequences in: ${runtime}" >> runtime.log



                                                 
