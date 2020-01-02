#!/usr/bin/env bash
set -o errexit
set -o nounset

HOST="$(hostname)"
if [[ ${HOST} =~ ukko ]]; then
  echo "In ukko"
  source ./local_ukko.tcl
else
  echo "Normal compile..."
fi


DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )
TARGET_FOLDER="${DIR}/sdsl-local-install"

if [ ! -d "${TARGET_FOLDER}" ]; then

  echo "*********************************"
  echo "Compiling and installing sdsl..."
  echo "*********************************"

  wget https://github.com/simongog/sdsl-lite/archive/v2.0.3.zip
  unzip v2.0.3.zip
  mkdir -p ${TARGET_FOLDER}
  cd sdsl-lite-2.0.3/
  ./install.sh ${TARGET_FOLDER}
  cd ..

  rm v2.0.3.zip
else
  echo "SDSL ALREADY INSTALLED"
fi
echo "SDSL READY..."
#LZscan does not need to be compile

TO_COMPILE="${DIR}/BWA/bwa-0.7.12/"

echo "Compiling ${TO_COMPILE}..."
cd ${TO_COMPILE}; make;
echo "${TO_COMPILE} ready..."
cd ${DIR}

TO_COMPILE="${DIR}/BOWTIE2/bowtie2-2.2.9/"

echo "Compiling ${TO_COMPILE}..."
cd ${TO_COMPILE}; make;
echo "${TO_COMPILE} ready..."
cd ${DIR}

TO_COMPILE="${DIR}/LZ/"
echo "Compiling ${TO_COMPILE}..."
cd ${TO_COMPILE}; make;
echo "${TO_COMPILE} ready..."
cd ${DIR}


