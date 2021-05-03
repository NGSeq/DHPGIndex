#!/usr/bin/env bash

CUR=$( pwd )

# compile needed accessories

#Install following libraries (CentOS7)
#sudo yum group install "Development Tools"
#sudo yum install libstdc++-static
#sudo yum install maven
#sudo yum install cmake
#sudo yum install unzip


#cd modules/chic
 
#make

cd ext/LZ/RLZ_parallel/src

make

cd ${CUR}

cd modules/chic/src
make all
cd ${CUR}

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
cp sbin/index_chr.sh index/
cp sbin/index_partition.sh index/

mkdir -p /opt/dhpgindex
cp index/* /opt/dhpgindex/

mvn package


