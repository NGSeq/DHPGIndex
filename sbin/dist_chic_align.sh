#!/usr/bin/env bash
set -e 
set -o pipefail

align() {
    i=$(printf "%02d" $1)

    ssh  -tt -o "StrictHostKeyChecking no" node-$1 hdfs dfs -get reads_1/* /mnt/tmp/reads_1.fq&
    ssh  -tt -o "StrictHostKeyChecking no" node-$1 hdfs dfs -get reads_2/* /mnt/tmp/reads_2.fq
    ssh  -tt -o "StrictHostKeyChecking no" node-$1 /opt/dhpgindex/chic_align -v1 -t 16 -o /mnt/tmp/aligned.sam /mnt/tmp/part-${i}.fa /mnt/tmp/reads_1.fq /mnt/tmp/reads_2.fq &> chic_align$i.log
    ssh  -tt -o "StrictHostKeyChecking no" node-$1 /opt/samtools/samtools view -F4 /mnt/tmp/aligned.sam > /mnt/tmp/mapped$i.sam 
    scp -o "StrictHostKeyChecking no" node-$1:/mnt/tmp/mapped$i.sam sams/
     
}

for num in {1..22}; do align "$num"& done

