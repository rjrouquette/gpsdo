#!/bin/bash -e

cd "$(dirname "${BASH_SOURCE[0]}")"

export PATH=/opt/ti/msp430-gcc/bin:$PATH

mkdir -p dist/
mkdir -p build/
rm -f dist/*
cd build

CORES=$(nproc)
echo "Running build with $CORES threads."

rm -rf ./*
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=msp430-elf-gcc -DCMAKE_CXX_COMPILER=msp430-elf-g++ ..
#make VERBOSE=1
make -j$CORES
mv -v ./gpsdo ../dist

cd ..
rm -rf build
msp430-elf-objcopy -vO ihex dist/gpsdo dist/gpsdo.hex

echo "Build Successful"
exit 0
