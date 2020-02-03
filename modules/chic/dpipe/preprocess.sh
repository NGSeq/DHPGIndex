#!/usr/bin/env bash

mkdir -p /media/tmp/all/headers/
mkdir -p /media/tmp/all/pg/
mkdir -p /media/tmp/all/pg/

task(){ 
      i=$(printf "%02d" $1)
       mkdir -p /media/tmp/all/$i
        cd /media/tmp/all/$i
        /root/vcf2multialign -S --chunk-size=200 -r /media/msa/hg19/chr${1}_u.fa -a /media/msa/vcfsubsets/subset${1}_1.vcf
 mmv "./*" "./#1.${i}"

 #hdfs dfs -put /media/tmp/all/$i/* /user/root/all
 #rm -f /media/tmp/all/$i/*
/root/vcf2multialign -S --chunk-size=200 -r /media/msa/hg19/chr${1}_u.fa -a /media/msa/vcfsubsets/subset${1}_2.vcf
 mmv "./*" "./#1.${i}"
 #hdfs dfs -put /media/tmp/all/$i/* /user/root/all
 #rm -f /media/tmp/all/$i/*
/root/vcf2multialign -S --chunk-size=200 -r /media/msa/hg19/chr${1}_u.fa -a /media/msa/vcfsubsets/subset${1}_3.vcf
 mmv "./*" "./#1.${i}"

 #hdfs dfs -put /media/tmp/all/$i/* /user/root/all
 #rm -f /media/tmp/all/$i/*
 /root/vcf2multialign -S --chunk-size=200 -r /media/msa/hg19/chr${1}_u.fa -a /media/msa/vcfsubsets/subset${1}_4.vcf
 mmv "./*" "./#1.${i}"
 
 #hdfs dfs -put /media/tmp/all/$i/* /user/root/all
 #rm -f /media/tmp/all/$i/*
 /root/vcf2multialign -S --chunk-size=200 -r /media/msa/hg19/chr${1}_u.fa -a /media/msa/vcfsubsets/subset${1}_5.vcf
 mmv "./*" "./#1.${i}"
 #gzip *
 #hdfs dfs -put /media/tmp/all/$i/* /user/root/all
 #rm -f /media/tmp/all/$i/*
 
 ####create fasta headers###
 for f in *; do echo ">$f" > /media/tmp/all/hdfs/${f}a ; done
 
 mmv "./*" "/media/tmp/all/pg/#1.${i}b" # Same files as .gapped in org PanVC
}

for num in {1..22}; do task "$num"& done
