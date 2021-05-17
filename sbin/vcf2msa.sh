#!/bin/bash

#Download latest from https://github.com/tsnorri/vcf2multialign

PGPATHHDFS=$1
PGPATHLOCAL=$2
STDREFPATH=$3
VCFPATH=$4
CHR=$5

mkdir -p $PGPATHLOCAL

DIR=$( pwd )

      i=$(printf "%02d" $CHR)
       mkdir -p $PGPATHLOCAL/$i
       cd $PGPATHLOCAL/$i
       $DIR/vcf2multialign -S --chunk-size=200 -r $STDREFPATH/chr${CHR}.fa -a $VCFPATH/chr${CHR}.vcf

      # Add Fasta header
      ls | xargs -I{} -n 1 -P16 sed -i 1i">"{}".${i}" {}

      mmv "./*" "../#1.${i}"
      # Concate by chromosomes if needed
      #cat "./*" >> "../chr.${i}"

      hdfs dfs -put $PGPATHLOCAL/*.$i $PGPATHHDFS/
      #rm -rf $PGPATHLOCAL/$i/





