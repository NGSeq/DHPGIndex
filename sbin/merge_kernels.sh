#!/usr/bin/env bash

LOCALMERGEDINDEXPATH=$1
PARTS=$2
MAX_QUERY_LEN=$3
ALIGNER=BOWTIE2
threads=16

metaf=""
limitf=""
lzs=""

DIR=$( pwd )
cd $LOCALMERGEDINDEXPATH

for (( c=1; c<=$PARTS; c++ ))
  do
   i=$(printf "%02d" $c)
   metaf="${metaf} chr${i}.fa.metadata "
   limitf="${limitf} chr${i}.fa.limits_kernel "
   lzs="${lzs} chr${i}.lz "

  done

echo "/opt/dhpgindex/merge_meta_offsets $metaf"
echo "/opt/dhpgindex/merge_with_offsets $lzs"
/opt/dhpgindex/merge_meta_offsets $metaf
/opt/dhpgindex/merge_with_offsets $lzs

mv merged.meta merged_index.metadata

echo ">Dummy_header." >> merged_index.kernel_text

for (( c=1; c<=$PARTS; c++ ))
  do
    i=$(printf "%02d" $c)
    sed '/^>/d' chr$i.fa.kernel_text >> merged_index.kernel_text
  done

echo "/opt/dhpgindex/chic_index --threads=${threads}  --kernel=${ALIGNER} --verbose=2 --indexing --lz-parsing-method=RLZ --lz-input-plain-file=merged_phrases.lz merged_index ${MAX_QUERY_LEN} &> indexing_merged.log"
/opt/dhpgindex/chic_index --threads=${threads} --kernel=${ALIGNER} --verbose=2 --indexing --lz-parsing-method=RLZ --lz-input-plain-file=merged_phrases.lz merged_index ${MAX_QUERY_LEN} &> indexing_merged.log

/opt/dhpgindex/chic_map 0 $limitf
/opt/dhpgindex/chic_map 1 merged.limits_kernel
mv merged.limits_kernel merged_index.limits_kernel
mv merged.sparse_limits_kernel merged_index.sparse_sample_limits_kernel