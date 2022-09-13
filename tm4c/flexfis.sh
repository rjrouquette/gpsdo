#!/bin/bash
set -e
g++ -ggdb3 -o dist/flexfis-test qnorm.c flexfis.c flex_test.cpp
./dist/flexfis-test ../fpm-test/gpsdo.dcxo.csv
echo "[Done]"
