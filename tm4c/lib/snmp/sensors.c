//
// Created by robert on 5/3/23.
//

#include <math.h>
#include <string.h>
#include "../clk/mono.h"
#include "../ntp/pll.h"
#include "../ntp/tcmp.h"
#include "sensors.h"
#include "util.h"

static const uint8_t OID_SENSOR_PREFIX[] = { 0x06, 0x0A, 0x2B, 6, 1, 2, 1, 99, 1, 1, 1 };


int SNMP_writeSensorTypes(uint8_t * const dst) {
    uint8_t *ptr = dst;

    // processor temperature
    ptr += SNMP_writeSensorType(ptr, OID_SENSOR_TYPE_CELSIUS);
    // PLL offset stats
    ptr += SNMP_writeSensorType(ptr, OID_SENSOR_TYPE_OTHER);
    ptr += SNMP_writeSensorType(ptr, OID_SENSOR_TYPE_OTHER);
    ptr += SNMP_writeSensorType(ptr, OID_SENSOR_TYPE_OTHER);
    ptr += SNMP_writeSensorType(ptr, OID_SENSOR_TYPE_OTHER);
    ptr += SNMP_writeSensorType(ptr, OID_SENSOR_TYPE_OTHER);
    ptr += SNMP_writeSensorType(ptr, OID_SENSOR_TYPE_OTHER);
    ptr += SNMP_writeSensorType(ptr, OID_SENSOR_TYPE_OTHER);
    // PLL drift stats
    ptr += SNMP_writeSensorType(ptr, OID_SENSOR_TYPE_OTHER);
    ptr += SNMP_writeSensorType(ptr, OID_SENSOR_TYPE_OTHER);
    ptr += SNMP_writeSensorType(ptr, OID_SENSOR_TYPE_OTHER);
    ptr += SNMP_writeSensorType(ptr, OID_SENSOR_TYPE_OTHER);
    ptr += SNMP_writeSensorType(ptr, OID_SENSOR_TYPE_OTHER);
    ptr += SNMP_writeSensorType(ptr, OID_SENSOR_TYPE_OTHER);
    ptr += SNMP_writeSensorType(ptr, OID_SENSOR_TYPE_OTHER);

    return ptr - dst;
}

int SNMP_writeSensorScales(uint8_t * const dst) {
    uint8_t *ptr = dst;

    // processor temperature
    ptr += SNMP_writeSensorScale(ptr, OID_SENSOR_SCALE_1);
    // PLL offset stats
    ptr += SNMP_writeSensorScale(ptr, OID_SENSOR_SCALE_1E_6);
    ptr += SNMP_writeSensorScale(ptr, OID_SENSOR_SCALE_1E_6);
    ptr += SNMP_writeSensorScale(ptr, OID_SENSOR_SCALE_1E_6);
    ptr += SNMP_writeSensorScale(ptr, OID_SENSOR_SCALE_1E_6);
    ptr += SNMP_writeSensorScale(ptr, OID_SENSOR_SCALE_1E_6);
    ptr += SNMP_writeSensorScale(ptr, OID_SENSOR_SCALE_1E_6);
    ptr += SNMP_writeSensorScale(ptr, OID_SENSOR_SCALE_1E_6);
    // PLL drift stats
    ptr += SNMP_writeSensorScale(ptr, OID_SENSOR_SCALE_1E_6);
    ptr += SNMP_writeSensorScale(ptr, OID_SENSOR_SCALE_1E_6);
    ptr += SNMP_writeSensorScale(ptr, OID_SENSOR_SCALE_1E_6);
    ptr += SNMP_writeSensorScale(ptr, OID_SENSOR_SCALE_1E_6);
    ptr += SNMP_writeSensorScale(ptr, OID_SENSOR_SCALE_1E_6);
    ptr += SNMP_writeSensorScale(ptr, OID_SENSOR_SCALE_1E_6);
    ptr += SNMP_writeSensorScale(ptr, OID_SENSOR_SCALE_1E_6);

    return ptr - dst;
}

int SNMP_writeSensorPrecs(uint8_t * const dst) {
    uint8_t *ptr = dst;

    // processor temperature
    ptr += SNMP_writeSensorPrec(ptr, 4);
    // PLL offset stats
    ptr += SNMP_writeSensorPrec(ptr, 4);
    ptr += SNMP_writeSensorPrec(ptr, 4);
    ptr += SNMP_writeSensorPrec(ptr, 4);
    ptr += SNMP_writeSensorPrec(ptr, 4);
    ptr += SNMP_writeSensorPrec(ptr, 4);
    ptr += SNMP_writeSensorPrec(ptr, 4);
    ptr += SNMP_writeSensorPrec(ptr, 4);
    // PLL drift stats
    ptr += SNMP_writeSensorPrec(ptr, 4);
    ptr += SNMP_writeSensorPrec(ptr, 4);
    ptr += SNMP_writeSensorPrec(ptr, 4);
    ptr += SNMP_writeSensorPrec(ptr, 4);
    ptr += SNMP_writeSensorPrec(ptr, 4);
    ptr += SNMP_writeSensorPrec(ptr, 4);
    ptr += SNMP_writeSensorPrec(ptr, 4);

    return ptr - dst;
}

int SNMP_writeSensorValues(uint8_t * const dst) {
    uint8_t *ptr = dst;

    // processor temperature
    ptr += SNMP_writeSensorValue(ptr, lroundf(TCMP_temp() * 1e4f));
    // PLL offset stats
    ptr += SNMP_writeSensorValue(ptr, lroundf(PLL_offsetLast() * 1e10f));
    ptr += SNMP_writeSensorValue(ptr, lroundf(PLL_offsetMean() * 1e10f));
    ptr += SNMP_writeSensorValue(ptr, lroundf(PLL_offsetRms() * 1e10f));
    ptr += SNMP_writeSensorValue(ptr, lroundf(PLL_offsetStdDev() * 1e10f));
    ptr += SNMP_writeSensorValue(ptr, lroundf(PLL_offsetProp() * 1e10f));
    ptr += SNMP_writeSensorValue(ptr, lroundf(PLL_offsetInt() * 1e10f));
    ptr += SNMP_writeSensorValue(ptr, lroundf(PLL_offsetCorr() * 1e10f));
    // PLL drift stats
    ptr += SNMP_writeSensorValue(ptr, lroundf(PLL_driftLast() * 1e10f));
    ptr += SNMP_writeSensorValue(ptr, lroundf(PLL_driftMean() * 1e10f));
    ptr += SNMP_writeSensorValue(ptr, lroundf(PLL_driftRms() * 1e10f));
    ptr += SNMP_writeSensorValue(ptr, lroundf(PLL_driftStdDev() * 1e10f));
    ptr += SNMP_writeSensorValue(ptr, lroundf(PLL_driftInt() * 1e10f));
    ptr += SNMP_writeSensorValue(ptr, lroundf(PLL_driftTcmp() * 1e10f));
    ptr += SNMP_writeSensorValue(ptr, lroundf(PLL_driftCorr() * 1e10f));

    return ptr - dst;
}

int SNMP_writeSensorStatuses(uint8_t * const dst) {
    uint8_t *ptr = dst;

    // processor temperature
    ptr += SNMP_writeSensorStatus(ptr, OID_SENSOR_STATUS_OK);
    // PLL mean offset
    ptr += SNMP_writeSensorStatus(ptr, OID_SENSOR_STATUS_OK);
    // PLL RMS offset
    ptr += SNMP_writeSensorStatus(ptr, OID_SENSOR_STATUS_OK);
    // PLL RMS skew
    ptr += SNMP_writeSensorStatus(ptr, OID_SENSOR_STATUS_OK);
    // PLL correction
    ptr += SNMP_writeSensorStatus(ptr, OID_SENSOR_STATUS_OK);
    // PLL temperature compensation bias
    ptr += SNMP_writeSensorStatus(ptr, OID_SENSOR_STATUS_OK);
    // PLL temperature compensation coefficient
    ptr += SNMP_writeSensorStatus(ptr, OID_SENSOR_STATUS_OK);
    // PLL temperature compensation value
    ptr += SNMP_writeSensorStatus(ptr, OID_SENSOR_STATUS_OK);

    return ptr - dst;
}

int SNMP_writeSensorUnits(uint8_t * const dst) {
    uint8_t *ptr = dst;

    // processor temperature
    ptr += SNMP_writeSensorUnit(ptr, "C");
    // PLL offset stats
    ptr += SNMP_writeSensorUnit(ptr, "s");
    ptr += SNMP_writeSensorUnit(ptr, "s");
    ptr += SNMP_writeSensorUnit(ptr, "s");
    ptr += SNMP_writeSensorUnit(ptr, "s");
    ptr += SNMP_writeSensorUnit(ptr, "s/s");
    ptr += SNMP_writeSensorUnit(ptr, "s/s");
    ptr += SNMP_writeSensorUnit(ptr, "s/s");
    // PLL drift stats
    ptr += SNMP_writeSensorUnit(ptr, "s/s");
    ptr += SNMP_writeSensorUnit(ptr, "s/s");
    ptr += SNMP_writeSensorUnit(ptr, "s/s");
    ptr += SNMP_writeSensorUnit(ptr, "s/s");
    ptr += SNMP_writeSensorUnit(ptr, "s/s");
    ptr += SNMP_writeSensorUnit(ptr, "s/s");
    ptr += SNMP_writeSensorUnit(ptr, "s/s");

    return ptr - dst;
}

int SNMP_writeSensorUpdateTimes(uint8_t * const dst) {
    uint8_t *ptr = dst;

    uint32_t timeTicks = ((CLK_MONO() * 100) >> 32);

    // processor temperature
    ptr += SNMP_writeSensorUpdateTime(ptr, timeTicks);
    // PLL offset stats
    ptr += SNMP_writeSensorUpdateTime(ptr, timeTicks);
    ptr += SNMP_writeSensorUpdateTime(ptr, timeTicks);
    ptr += SNMP_writeSensorUpdateTime(ptr, timeTicks);
    ptr += SNMP_writeSensorUpdateTime(ptr, timeTicks);
    ptr += SNMP_writeSensorUpdateTime(ptr, timeTicks);
    ptr += SNMP_writeSensorUpdateTime(ptr, timeTicks);
    ptr += SNMP_writeSensorUpdateTime(ptr, timeTicks);
    // PLL drift stats
    ptr += SNMP_writeSensorUpdateTime(ptr, timeTicks);
    ptr += SNMP_writeSensorUpdateTime(ptr, timeTicks);
    ptr += SNMP_writeSensorUpdateTime(ptr, timeTicks);
    ptr += SNMP_writeSensorUpdateTime(ptr, timeTicks);
    ptr += SNMP_writeSensorUpdateTime(ptr, timeTicks);
    ptr += SNMP_writeSensorUpdateTime(ptr, timeTicks);
    ptr += SNMP_writeSensorUpdateTime(ptr, timeTicks);

    return ptr - dst;
}

int SNMP_writeSensorUpdateRates(uint8_t * const dst) {
    uint8_t *ptr = dst;

    // processor temperature
    ptr += SNMP_writeSensorUpdateRate(ptr, 0);
    // PLL offset stats
    ptr += SNMP_writeSensorUpdateRate(ptr, 0);
    ptr += SNMP_writeSensorUpdateRate(ptr, 0);
    ptr += SNMP_writeSensorUpdateRate(ptr, 0);
    ptr += SNMP_writeSensorUpdateRate(ptr, 0);
    ptr += SNMP_writeSensorUpdateRate(ptr, 0);
    ptr += SNMP_writeSensorUpdateRate(ptr, 0);
    ptr += SNMP_writeSensorUpdateRate(ptr, 0);
    // PLL drift stats
    ptr += SNMP_writeSensorUpdateRate(ptr, 0);
    ptr += SNMP_writeSensorUpdateRate(ptr, 0);
    ptr += SNMP_writeSensorUpdateRate(ptr, 0);
    ptr += SNMP_writeSensorUpdateRate(ptr, 0);
    ptr += SNMP_writeSensorUpdateRate(ptr, 0);
    ptr += SNMP_writeSensorUpdateRate(ptr, 0);
    ptr += SNMP_writeSensorUpdateRate(ptr, 0);

    return ptr - dst;
}

int SNMP_writeSensorNames(uint8_t * const dst) {
    uint8_t *ptr = dst;

    // processor temperature
    ptr += SNMP_writeSensorName(ptr, "cpu.temp");
    // PLL offset stats
    ptr += SNMP_writeSensorName(ptr, "pll.offset.last");
    ptr += SNMP_writeSensorName(ptr, "pll.offset.mean");
    ptr += SNMP_writeSensorName(ptr, "pll.offset.rms");
    ptr += SNMP_writeSensorName(ptr, "pll.offset.stddev");
    ptr += SNMP_writeSensorName(ptr, "pll.offset.prop");
    ptr += SNMP_writeSensorName(ptr, "pll.offset.int");
    ptr += SNMP_writeSensorName(ptr, "pll.offset.corr");
    // PLL drift stats
    ptr += SNMP_writeSensorName(ptr, "pll.drift.last");
    ptr += SNMP_writeSensorName(ptr, "pll.drift.mean");
    ptr += SNMP_writeSensorName(ptr, "pll.drift.rms");
    ptr += SNMP_writeSensorName(ptr, "pll.drift.stddev");
    ptr += SNMP_writeSensorName(ptr, "pll.drift.int");
    ptr += SNMP_writeSensorName(ptr, "pll.drift.tcmp");
    ptr += SNMP_writeSensorName(ptr, "pll.drift.corr");

    return ptr - dst;
}



int SNMP_writeSensorType(uint8_t * const dst, int typeId) {
    return SNMP_writeValueInt8(dst, OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX), OID_SENSOR_TYPE, typeId);
}

int SNMP_writeSensorScale(uint8_t * const dst, int scaleId) {
    return SNMP_writeValueInt8(dst, OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX), OID_SENSOR_SCALE, scaleId);
}

int SNMP_writeSensorPrec(uint8_t * const dst, int precision) {
    return SNMP_writeValueInt8(dst, OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX), OID_SENSOR_PREC, precision);
}

int SNMP_writeSensorValue(uint8_t * const dst, int value) {
    return SNMP_writeValueInt32(dst, OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX), OID_SENSOR_VALUE, value);
}

int SNMP_writeSensorStatus(uint8_t * const dst, int statusId) {
    return SNMP_writeValueInt32(dst, OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX), OID_SENSOR_STATUS, statusId);
}

int SNMP_writeSensorUnit(uint8_t * const dst, const char *unit) {
    return SNMP_writeValueBytes(
            dst, OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX), OID_SENSOR_UNITS, unit, (int) strlen(unit)
    );
}

int SNMP_writeSensorUpdateTime(uint8_t * const dst, uint32_t timeTicks) {
    return SNMP_writeValueInt32(dst, OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX), OID_SENSOR_UPDATE_TIME, timeTicks);
}

int SNMP_writeSensorUpdateRate(uint8_t * const dst, uint32_t millis) {
    return SNMP_writeValueInt32(dst, OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX), OID_SENSOR_UPDATE_RATE, millis);
}

int SNMP_writeSensorName(uint8_t * const dst, const char *name) {
    return SNMP_writeValueBytes(
            dst, OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX), OID_SENSOR_NAME, name, (int) strlen(name)
    );
}
