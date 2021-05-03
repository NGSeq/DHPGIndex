#!/usr/bin/env bash

set -e
#set -o pipefail
#set -v
#set -x

export SPARK_MAJOR_VERSION=2

PGPATHLOCAL=$1
PGPATHHDFS=pg
DRLZPATHHDFS=drlz

READS_1=$2
READS_2=$3

#USE with BLAST
#QSEQS=$2

#Put read files to HDFS if distributed alignment is preferred
#hdfs dfs -mkdir -p reads_1&
#hdfs dfs -mkdir -p reads_2&
#hdfs dfs -put $READS_1 reads_1&
#hdfs dfs -put $READS_2 reads_2&
#OR with BLAST
#hdfs dfs -mkdir -p qseqs&
#hdfs dfs -put $QSEQS qseqs/&

LOCALINDEXPATH=/mnt/tmp
STDREFPATH=/data/grch37 #FASTA files must be divided by chromosomes and named chrN.fa N=1..22
VCFPATH=/data/vcfs #VCF files must be divided by chromosomes and named chrN.vcf N=1..22
HDFSURI=hdfs://namenode:8020 # HDFS URI
NODE=node- #Basename for nodes in the cluster. The nodes must be numbered starting from 1.
SPARKMASTER=yarn-client #For single node testing use local[n] where n is the amount of executors,
N=26 #Number of cluster nodes
MAX_QUERY_LEN=102 # Set maximum sequence length for index queries (shoukd be > max read length)
DICTIONARYSIZE=75 #The number of genomes or sequences used as dictionary in compression
CHRS=22 #Number of chromosomes


hdfs dfs -mkdir -p $PGPATHHDFS
date >> runtime.log
echo "Started preparing pan-genome from VCF files with vcf2msa.."
start=`date +%s`
./vcf2msa.sh $PGPATHHDFS $PGPATHLOCAL $STDREFPATH $VCFPATH
end=`date +%s`
runtime=$((end-start))
echo "vcf2msa runtime: ${runtime}" >> runtime.log

date >> runtime.log
echo "Loading files to HDFS..."

date >> runtime.log
echo "Starting DRLZ.."
start=`date +%s`
seq 1 $CHRS | xargs -I{} -n 1 -P $CHRS ./drlz_hg.sh {} $PGPATHHDFS/ $DRLZPATHHDFS/ $DICTIONARYSIZE 40 50 30 $HDFSURI $SPARKMASTER
runtime=$((end-start))
echo "DRLZ compression time: ${runtime}" >> runtime.log

date >> runtime.log
echo "Starting distributed Kernelization.."


start=`date +%s`
cnt=0 #counter
nn=1 #node number
((d=$CHRS/$N))
for chr in {1..$CHRS}
do
  ((cnt=cnt+1))
  ssh -tt -o "StrictHostKeyChecking no" $NODE-$nn /opt/dhpgindex/index_chr.sh {} $PGPATHHDFS/pg/ $PGPATHHDFS/drlz --kernelize $MAX_QUERY_LEN
  if [[ "$cnt" == "$d" ]]; then
    ((cnt=0))
    ((nn=nn+1))
  fi
done

end=`date +%s`
runtime=$((end-start))
echo "Kernelized in: ${runtime}" >> runtime.log

date >> runtime.log
echo "Merging and indexing merged kernel.."
start=`date +%s`
mkdir -p $LOCALINDEXPATH/merged/
seq 1 $N | xargs -I{} -n 1 -P $N scp -o "StrictHostKeyChecking no" $NODE{}:$LOCALINDEXPATH/*.fa.* $LOCALINDEXPATH/merged/
merge_kernels.sh $LOCALINDEXPATH/merged $CHRS

/opt/dhpgindex/chic_index --threads=16  --kernel=BOWTIE2 --verbose=2 --indexing --lz-input-file=$LOCALINDEXPATH/merged/merged_phrases.lz $LOCALINDEXPATH/merged/ ${MAX_QUERY_LEN}

end=`date +%s`
runtime=$((end-start))
echo "Indexed in: ${runtime}" >> runtime.log

date >> runtime.log

echo "Aligning reads.."
start=`date +%s`
/opt/dhpgindex/chic_align -v1 -t 16 -o /mnt/tmp/aligned.sam $LOCALINDEXPATH/merged/merged $READS_1 $READS_2 #with BLAST: use single file defined in QSEQ variable
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



                                                 
