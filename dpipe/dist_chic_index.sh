#!/usr/bin/env bash
set -e 
set -o pipefail

HDFSGAPLESSPATH=$1gapless
HDFSLZPATH=$1drlz
index() {
  
    i=$(printf "%02d" $1)
    #ssh  -tt -o "StrictHostKeyChecking no" node-$1 hdfs dfs -getmerge $HDFSLZPATH/*.${i}.pos /mnt/tmp/chr$i.lz
    #ssh  -tt -o "StrictHostKeyChecking no" node-$1 hdfs dfs -get /user/root/reads_1/* /mnt/tmp/reads_1.fq&
    #ssh  -tt -o "StrictHostKeyChecking no" node-$1 hdfs dfs -get /user/root/reads_2/* /mnt/tmp/reads_2.fq
    
    #ssh  -tt -o "StrictHostKeyChecking no" node-$1 mkdir -p /mnt/tmp/hdfs
    #ssh  -tt -o "StrictHostKeyChecking no" node-$1 hdfs dfs -getmerge $HDFSGAPLESSPATH/*.$i* /mnt/tmp/hdfs
    #ssh  -tt -o "StrictHostKeyChecking no" node-$1 /opt/chic/rmgaps.sh 
    #ssh  -tt -o "StrictHostKeyChecking no" node-$1 /opt/chic/store_gaps /mnt/tmp/chr$i.fa
    
    ssh  -tt -o "StrictHostKeyChecking no" node-$1 /opt/chic/src/chic_index --hdfspath=$HDFSGAPLESSPATH/chr${i}.fa --kernelize=1 --indexing=1 --threads=16  --kernel=BOWTIE2 --verbose=2 --lz-input-file=/mnt/tmp/chr$i.lz -o /mnt/tmp/chr$i $HDFSGAPLESSPATH/chr${i}.fa/part-00000 150
    ssh  -tt -o "StrictHostKeyChecking no" node-$1 /opt/chic/src/chic_align -v1 -t 16 -o /mnt/tmp/aligned.sam /mnt/tmp/chr$i /mnt/tmp/reads_1.fq /mnt/tmp/reads_2.fq
    ssh  -tt -o "StrictHostKeyChecking no" node-$1 /opt/samtools/samtools view -F4 /mnt/tmp/aligned.sam > /mnt/tmp/mapped$i.sam 
    #scp -o "StrictHostKeyChecking no" node-$1:/mnt/tmp/mapped$i.sam sams/
     
}

for num in {21..22}; do index "$num"& done

