#!/usr/bin/env bash
set -o errexit
set -o nounset

DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

TO_COMPILE="${DIR}/BWA/bwa-0.7.12/"

echo "Compiling ${TO_COMPILE}..."
cd ${TO_COMPILE}; make clean;
echo "${TO_COMPILE} ready..."
cd ${DIR}

TO_COMPILE="${DIR}/LZ/"
echo "Compiling ${TO_COMPILE}..."
cd ${TO_COMPILE}; make clean;
echo "${TO_COMPILE} ready..."
cd ${DIR}

## CLEAN SDSL IS THE LAST THING TO BE DONE
rm -rf "sdsl-lite-2.0.3/"
rm -rf "sdsl-local-install/"
rm -rf "v2.0.3.zip"

