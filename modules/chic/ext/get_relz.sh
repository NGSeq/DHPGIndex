#!/usr/bin/env bash
set -o errexit
set -o nounset

DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

TARGET_FOLDER="${DIR}/ReLZ"
if [ ! -d "${TARGET_FOLDER}" ]; then

  echo "*********************************"
  echo "Compiling and installing ReLZ"
  echo "*********************************"
  
  COMMIT="master"
  git clone https://gitlab.com/dvalenzu/ReLZ.git
	cd ReLZ;
  git checkout ${COMMIT}
  sed  -i -e s/\$\(PARANOID\)// ./src/Makefile
  sed  -i -e s/make/ls/ ./ext/setup_fse.sh
#  sed  -i -e s/\$\(LIBFASTPFOR\)// ./src/Makefile
#  sed  -i -e s/cmake/ls/ ./ext/setup_fastpfor.sh
#  sed  -i -e s/make/ls/ ./ext/setup_fastpfor.sh
	
  make;
	cd ..;
else
  echo "ReLZ ALREADY INSTALLED"
fi

