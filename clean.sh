#!/usr/bin/env bash

CUR=$( pwd )

cd modules/chic/ext/

make clean

cd ${CUR}

cd modules/chic/src
make clean
cd ${CUR}

cd modules/LZ-tools

make clean


rm -rf /opt/dhpgindex/

cd ${CUR}

mvn clean
