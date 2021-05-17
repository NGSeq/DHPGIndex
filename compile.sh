#!/usr/bin/env bash

CUR=$( pwd )

# compile needed accessories

#Install following libraries (CentOS7)
sudo yum -y group install "Development Tools"
sudo yum -y install libstdc++-static
sudo yum -y install maven
sudo yum -y install cmake
sudo yum -y install unzip
sudo yum -y install mmv
sudo yum -y install curl-devel

cd modules/chic/ext/

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

mkdir -p /opt/dhpgindex

cp modules/chic/src/chic_index /opt/dhpgindex/
cp modules/chic/src/chic_align /opt/dhpgindex/
cp modules/chic/src/chic_map /opt/dhpgindex/
cp modules/LZ-tools/merge_limits /opt/dhpgindex/
cp modules/LZ-tools/merge_meta_offsets /opt/dhpgindex/
cp modules/LZ-tools/merge_with_offsets /opt/dhpgindex/
cp sbin/index_chr.sh /opt/dhpgindex/
cp sbin/index_partition.sh /opt/dhpgindex/


mvn package


