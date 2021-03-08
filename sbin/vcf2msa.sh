#!/usr/bin/env bash

#Download latest from https://github.com/tsnorri/vcf2multialign

PGPATHHDFS=$1
PGPATHLOCAL=$2
STDREFPATH=$3
VCFPATH=$4

mkdir -p $PGPATHLOCAL

DIR=$( pwd )
task(){ 
      i=$(printf "%02d" $1)
       mkdir -p $PGPATHLOCAL/$i
       cd $PGPATHLOCAL/$i
       $DIR/sbin/vcf2multialign -S --chunk-size=200 -r $STDREFPATH/chr${1}.fa -a $VCFPATH/chr${1}.vcf
      mmv "./*" "../#1.${i}"

      hdfs dfs -put $PGPATHLOCAL/*.$i $PGPATHHDFS
      #rm -rf $PGPATHLOCAL/$i/
}
for num in {1..22}; do task "$num"& done

