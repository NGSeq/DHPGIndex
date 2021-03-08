#!/usr/bin/env bash

LOCALMERGEDINDEXPATH=$1
ALIGNER=BOWTIE2
MAX_QUERY_LEN=102
cpus=$(lscpu -p | wc -l)

metaf=""
limitf=""
lzs=""

for i in {01..22}
  do
   metaf=${metaf} $LOCALMERGEDINDEXPATH/"chr"${i}".fa.metadata "
   limitf=${limitf} $LOCALMERGEDINDEXPATH/"chr"${i}".fa.limits_kernel "
   lzs=${lzs} $LOCALMERGEDINDEXPATH/"chr"${i}".lz "

  done

DIR=$( pwd )
cd $LOCALMERGEDINDEXPATH

$DIR/sbin/merge_meta_offsets $metaf

$DIR/sbin/merge_with_offsets $lzs

echo ">Dummy_header." >> merged_index.P512_GC4_kernel_text

for i in {01..22}
  do
    sed '/^>/d' chr$i.fa.P512_GC4_kernel_text >> merged_index.P512_GC4_kernel_text

  done

 /opt/chic/src/chic_index --threads=${cpus}  --kernel=${ALIGNER} --verbose=2 --lz-input-file=merged_phrases.lz merged ${MAX_QUERY_LEN}
 /opt/chic/src/chic_map 0 $limitf
 /opt/chic/src/chic_map 1 merged.limits_kernel