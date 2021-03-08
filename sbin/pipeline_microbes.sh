#!/usr/bin/env bash

#set -e
#set -o pipefail
set -v
set -x

export SPARK_MAJOR_VERSION=2

PGPATHLOCAL=$1
PGPATHHDFS=pg

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
HDFSURI=hdfs://namenode:8020 # HDFS URI
NODE=node- #Basename for nodes in the cluster. The nodes must be numbered starting from 1.
MAX_QUERY_LEN=1000 # Set maximum sequence length for index queries (shoukd be > max read length)

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
seq 1 25 | xargs -I{} -n 1 -P 5 ./drlz_microbes.sh {} pg lz groupedfasta 0.33 &
runtime=$((end-start))
echo "DRLZ compression time: ${runtime}" >> runtime.log

date >> runtime.log
echo "Starting distributed Kernelization.."
start=`date +%s`
seq 1 25 | xargs -I{} -n 1 -P 22 ssh -tt -o "StrictHostKeyChecking no" $NODE{} /opt/chic/index/index_partition.sh {} $PGPATHHDFS/pg/ $PGPATHHDFS/drlz --kernelize

end=`date +%s`
runtime=$((end-start))
echo "Kernelized in: ${runtime}" >> runtime.log

date >> runtime.log
echo "Merging and indexing merged kernel.."
start=`date +%s`
mkdir -p $LOCALINDEXPATH/merged/
seq 1 25 | xargs -I{} -n 1 -P 25 scp -o "StrictHostKeyChecking no" $NODE{}:$LOCALINDEXPATH/part-${i}.fa.* $LOCALINDEXPATH/merged/
#With BLAST use
#./merge_blast_index.sh $LOCALINDEXPATH/merged
./merge_chr_index.sh $LOCALINDEXPATH/merged

#Comment the next line out with BLAST, indexing should have been done in parallel and merged in the previous step
/opt/chic/index/chic_index --threads=16  --kernel=BOWTIE2 --verbose=2 --indexing --lz-input-file=$LOCALINDEXPATH/merged/merged_phrases.lz $LOCALINDEXPATH/merged/ ${MAX_QUERY_LEN}

end=`date +%s`
runtime=$((end-start))
echo "Indexed in: ${runtime}" >> runtime.log

date >> runtime.log

echo "Aligning reads.."
start=`date +%s`
/opt/chic/index/chic_align -v1 -t 16 -o /mnt/tmp/aligned.sam $LOCALINDEXPATH/merged/merged $READS_1 $READS_2
/opt/samtools/samtools view -F4 /mnt/tmp/aligned.sam > /mnt/tmp/mapped.sam
end=`date +%s`
runtime=$((end-start))
echo "Aligned reads in: ${runtime}" >> runtime.log

#echo "Starting distributed alignment (per partition with BLAST).."
#start=`date +%s`
#./dist_chic_blast.sh $PGPATHHDFS
#end=`date +%s`
#runtime=$((end-start))
#echo "Aligned in : ${runtime}" >> runtime.log
#
#echo "Downloading alignment files to local FS.."
#start=`date +%s`
#
#mkdir -p $LOCALINDEXPATH/sams/
#seq 1 25 | xargs -I{} -n 1 -P 22 scp -o "StrictHostKeyChecking no" $NODE{}:$LOCALINDEXPATH/mapped $LOCALINDEXPATH/mapped
#
#end=`date +%s`
#runtime=$((end-start))
#echo "Downlaoded in : ${runtime}" >> runtime.log


                                                 
