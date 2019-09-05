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

cd ${CUR}

cd modules/radix

make

cd ${CUR}

#pip install numpy
#pip install biopython


cp modules/chic/ext/LZ/RLZ_parallel/src/rlz_for_hybrid rlz_for_hybrid
cp modules/chic/src/chic_index components/index/chic_index
cp modules/chic/src/chic_align components/index/chic_align
cp modules/chic/src/chic_map components/index/chic_map
cp modules/radix/radixSA radixSA

sbt compile

