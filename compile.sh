#!/usr/bin/env bash

CUR=$( pwd )

# compile needed accessories

cd modules/chic
 
make

cd ext/LZ/RLZ_parallel/src

make

cd ${CUR}

#cd modules/chic/src
#make all
#cd ${CUR}

cd modules/LZ-tools

make

cd ${CUR}

#pip install numpy
#pip install biopython

cp modules/chic/src/chic_index index/chic_index
cp modules/chic/src/chic_align index/chic_align
cp modules/chic/src/chic_map index/chic_map
cp modules/LZ-tools/merge_limits index/
cp modules/LZ-tools/merge_meta_offsets index/
cp modules/LZ-tools/merge_with_offsets index/

sbt compile



