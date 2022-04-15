#!/bin/bash
set -e
cd "$(dirname "${BASH_SOURCE[0]}")"

lm4flash dist/gpsdo.bin
