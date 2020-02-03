#!/usr/bin/env bash
set -e 
set -o pipefail

#TODO store_gaps and remove gaps must be launched before puutting to HDFS as indexing is done from gap_pos files and PG without gaps
#store_gaps
#rmgaps.sh
#hdfs put gaps and gappless pg

HDFSPGPATH=$1
HDFSLZPATH=$2
index() {
    i=$(printf "%02d" $1)
    ssh  -tt -o "StrictHostKeyChecking no" node-$1 hdfs dfs -getmerge $HDFSLZPATH/*.$i.pos /mnt/tmp/chr$i.lz&
    ssh  -tt -o "StrictHostKeyChecking no" node-$1 hdfs dfs -get /user/root/reads_1/* /mnt/tmp/reads_1.fq&
    ssh  -tt -o "StrictHostKeyChecking no" node-$1 hdfs dfs -get /user/root/reads_2/* /mnt/tmp/reads_2.fq&
    ssh  -tt -o "StrictHostKeyChecking no" node-$1 hdfs dfs -getmerge $HDFSPGPATH/*.$i* /mnt/tmp/chr$i.fa  
    #ssh  -tt -o "StrictHostKeyChecking no" node-$1 /opt/chic/store_gaps /mnt/tmp/chr$i.fa
    ssh  -tt -o "StrictHostKeyChecking no" node-$1 /opt/chic/src/chic_index --threads=16  --kernel=BOWTIE2 --verbose=2 --lz-input-file=/mnt/tmp/chr$i.lz /mnt/tmp/chr$i.fa 150
    ssh  -tt -o "StrictHostKeyChecking no" node-$1 /opt/chic/src/chic_align -v1 -t 16 -o /mnt/tmp/aligned.sam /mnt/tmp/chr$i.fa /mnt/tmp/reads_1.fq /mnt/tmp/reads_2.fq
    ssh  -tt -o "StrictHostKeyChecking no" node-$1 /opt/samtools/samtools view -F4 /mnt/tmp/aligned.sam > /mnt/tmp/mapped$i.sam 
    scp -o "StrictHostKeyChecking no" node-$1:/mnt/tmp/mapped$i.sam sams/
     
}

for num in {1..22}; do index "$num"& done

