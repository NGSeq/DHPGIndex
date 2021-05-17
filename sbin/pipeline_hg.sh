#!/usr/bin/env bash

#set -e
#set -o pipefail
#set -v
#set -x

export SPARK_MAJOR_VERSION=2

PGPATHLOCAL=$1 #Local directory to store pan-genome files in FASTA format
READS_1=$2 #Local path to paired FASTQ file 1
READS_2=$3 #Local path to paired FASTQ file 2
#USE with BLAST
#QSEQS=$2 #Local path to single FASTA file (query file)

NODE=node- #Basename for nodes in the cluster. The nodes must be numbered starting from 1.
SPARKMASTER=yarn-client #For single node testing use local[n] where n is the amount of executors,
N=23 #Number of cluster nodes, N=1 is just for testing, HDFS requires at least 3 datanodes for replication as default

HDFSURI=hdfs://node-1:8020 # HDFS URI
HDFSUSERDIR=/user/root
PGPATHHDFS=$HDFSUSERDIR/pg #HDFS path to store pan-genome
DRLZPATHHDFS=$HDFSUSERDIR/drlz #HDFS path to store compressed data

LOCALINDEXPATH=/mnt/tmp #Intermediate files and the final index are stored here
STDREFPATH=/data/grch37 #FASTA files must be divided by chromosomes and named chrN.fa N=1..22
VCFPATH=/data/vcfs #VCF files must be divided by chromosomes and named chrN.vcf N=1..22

MAX_QUERY_LEN=102 # Set maximum sequence length for index queries (shoukd be > max read length)
DICTIONARYSIZE=75 #The number of genomes or sequences used as dictionary in compression
SPLITS=30 #To how many partitions chromosomes are split initially
EXECUTORMEM=50g  #Spark executor memory. If YARN used, cannot exceed yarn.nodemanager.resource.memory-mb defined in /opt/hadoop/etc/hadoop/yarn-site.xml
DRIVERMEM=50g  #Spark driver memory
THREADS=16 # Number of threads used with chic indexing and alignment (passed to Bowtie and BLAST parameters). Should not exceed the total number of cores of the node.
CHRS=22 #Number of chromosomes

hdfs dfs -mkdir -p $PGPATHHDFS
date >> runtime.log
echo "Started preparing pan-genome from VCF files with vcf2msa.."
start=`date +%s`
seq 1 $CHRS | xargs -I{} -n 1 -P$CHRS ./vcf2msa.sh $PGPATHHDFS $PGPATHLOCAL $STDREFPATH $VCFPATH {}
end=`date +%s`
runtime=$((end-start))
echo "vcf2msa runtime: ${runtime}" >> runtime.log

date >> runtime.log
echo "Loading files to HDFS..."

date >> runtime.log
echo "Starting DRLZ.."
start=`date +%s`
seq 1 $CHRS | xargs -I{} -n 1 -P $CHRS ./drlz_hg.sh {} $PGPATHHDFS/ $DRLZPATHHDFS/ $DICTIONARYSIZE $DRIVERMEM $EXECUTORMEM $SPLITS $HDFSURI $SPARKMASTER
runtime=$((end-start))
echo "DRLZ compression time: ${runtime}" >> runtime.log

date >> runtime.log
echo "Starting distributed Kernelization.."


start=`date +%s`
./distributed_kernelization.sh $PGPATHHDFS $DRLZPATHHDFS $LOCALINDEXPATH $MAX_QUERY_LEN $HDFSURI $SPARKMASTER "1-${CHRS}" $THREADS

end=`date +%s`
runtime=$((end-start))
echo "Kernelized in: ${runtime}" >> runtime.log

date >> runtime.log
echo "Merging and indexing merged kernel.."
start=`date +%s`
mkdir -p $LOCALINDEXPATH/merged/
seq 1 $N | xargs -I{} -n 1 -P $N scp -o "StrictHostKeyChecking no" $NODE{}:$LOCALINDEXPATH/*.fa.* $LOCALINDEXPATH/*.lz $LOCALINDEXPATH/merged/
./merge_kernels.sh $LOCALINDEXPATH/merged $CHRS ${MAX_QUERY_LEN}

end=`date +%s`
runtime=$((end-start))
echo "Indexed in: ${runtime}" >> runtime.log

date >> runtime.log

echo "Aligning reads.."
start=`date +%s`
/opt/dhpgindex/chic_align -v1 -t 16 -o /mnt/tmp/aligned.sam $LOCALINDEXPATH/merged/merged_index $READS_1 $READS_2 #with BLAST: use single file defined in QSEQ variable
#Comment next line out with BLAST
/opt/samtools/samtools view -F4 /mnt/tmp/aligned.sam > /mnt/tmp/mapped.sam
end=`date +%s`
runtime=$((end-start))
echo "Aligned reads in: ${runtime}" >> runtime.log

#Optional
#echo "Starting distributed alignment with BLAST(per chromosome).."
#start=`date +%s`
#./dist_chic_blast.sh $PGPATHHDFS
#end=`date +%s`
#runtime=$((end-start))
#echo "Aligned in : ${runtime}" >> runtime.log
#
#echo "Downloading SAM files to local FS.."
#start=`date +%s`
#
#mkdir -p $LOCALINDEXPATH/sams/
#seq 1 22 | xargs -I{} -n 1 -P 22 scp -o "StrictHostKeyChecking no" $NODE{}:$LOCALINDEXPATH/mapped*.sam $LOCALINDEXPATH/sams/
#
#end=`date +%s`
#runtime=$((end-start))
#echo "Downlaoded in : ${runtime}" >> runtime.log



                                                 
