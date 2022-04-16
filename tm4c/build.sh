#!/bin/bash
set -e
cd "$(dirname "${BASH_SOURCE[0]}")"

mkdir -p dist/
mkdir -p build/
rm -f dist/*
cd build

CORES=$(nproc)
echo "Running build with $CORES threads."

rm -fr ./*
cmake ..
#make VERBOSE=1
make -j$CORES
mv gpsdo* ../dist

cd ..
rm -fr build

gcc -o dist/ttf2bmp -I/usr/include/freetype2 ttf2bmp.c -lfreetype

exit 0
