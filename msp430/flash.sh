#!/bin/bash

# uncomment if library has not been registered with ldconfig
#export LD_LIBRARY_PATH=/opt/ti/MSPFlasher_1.3.20

# flash utility
MSPFlasher=/opt/ti/MSPFlasher_1.3.20/MSP430Flasher

${MSPFlasher} -i TIUSB
