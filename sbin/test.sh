#!/usr/bin/env bash
set -v

mkdir -p data/pg

CUR=$(pwd)
cd data/pg
wget https://ftp.ncbi.nlm.nih.gov/genomes/all/GCF/012/658/965/GCF_012658965.1_ASM1265896v1/GCF_012658965.1_ASM1265896v1_genomic.fna.gz
gunzip *.gz
head -n10 GCF_012658965.1_ASM1265896v1_genomic.fna > ../query.fna

cd $CUR

#Run the pipeline
./pipeline_microbes.sh data/pg/ data/query.fna

cat blasted/*