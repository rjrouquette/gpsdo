//
// Created by robert on 5/28/22.
//

#pragma once

#include <cstdint>

void GPS_init();

float GPS_locLat();
float GPS_locLon();
float GPS_locAlt();

int GPS_clkBias();
int GPS_clkDrift();
int GPS_accTime();
int GPS_accFreq();

int GPS_hasFix();

uint64_t GPS_taiEpochUpdate();
uint32_t GPS_taiEpoch();
int GPS_taiOffset();
