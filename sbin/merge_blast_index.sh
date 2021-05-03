#!/usr/bin/env bash

LOCALMERGEDINDEXPATH=$1
N=$2
DIR=$( pwd )

dbs=""
cd $LOCALMERGEDINDEXPATH
for i in {01..$N}
  do
   dbs=${dbs} "part-"${i}".fa "

  done

/opt/blast/bin/blastdb_aliastool -dbtype nucl -dblist "$dbs" -out merged

cd $DIR







