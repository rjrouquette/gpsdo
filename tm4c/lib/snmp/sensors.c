//
// Created by robert on 5/3/23.
//

#include <math.h>
#include <string.h>
#include "../clk/comp.h"
#include "../ntp/pll.h"
#include "../ntp/tcmp.h"
#include "sensors.h"
#include "util.h"
#include "../clk/mono.h"

static const uint8_t OID_SENSOR_PREFIX[] = { 0x06, 0x0A, 0x2B, 6, 1, 2, 1, 99, 1, 1, 1 };


int SNMP_writeSensorTypes(uint8_t * const dst) {
    uint8_t *ptr = dst;

    // processor temperature
    ptr += SNMP_writeSensorType(ptr, OID_SENSOR_TYPE_CELSIUS);
    // PLL mean offset
    ptr += SNMP_writeSensorType(ptr, OID_SENSOR_TYPE_OTHER);
    // PLL RMS offset
    ptr += SNMP_writeSensorType(ptr, OID_SENSOR_TYPE_OTHER);
    // PLL RMS skew
    ptr += SNMP_writeSensorType(ptr, OID_SENSOR_TYPE_OTHER);
    // PLL correction
    ptr += SNMP_writeSensorType(ptr, OID_SENSOR_TYPE_OTHER);
    // PLL temperature compensation bias
    ptr += SNMP_writeSensorType(ptr, OID_SENSOR_TYPE_OTHER);
    // PLL temperature compensation coefficient
    ptr += SNMP_writeSensorType(ptr, OID_SENSOR_TYPE_OTHER);
    // PLL temperature compensation value
    ptr += SNMP_writeSensorType(ptr, OID_SENSOR_TYPE_OTHER);

    return ptr - dst;
}

int SNMP_writeSensorScales(uint8_t * const dst) {
    uint8_t *ptr = dst;

    // processor temperature
    ptr += SNMP_writeSensorScale(ptr, OID_SENSOR_SCALE_1);
    // PLL mean offset
    ptr += SNMP_writeSensorScale(ptr, OID_SENSOR_SCALE_1E_9);
    // PLL RMS offset
    ptr += SNMP_writeSensorScale(ptr, OID_SENSOR_SCALE_1E_9);
    // PLL RMS skew
    ptr += SNMP_writeSensorScale(ptr, OID_SENSOR_SCALE_1E_9);
    // PLL correction
    ptr += SNMP_writeSensorScale(ptr, OID_SENSOR_SCALE_1);
    // PLL temperature compensation bias
    ptr += SNMP_writeSensorScale(ptr, OID_SENSOR_SCALE_1);
    // PLL temperature compensation coefficient
    ptr += SNMP_writeSensorScale(ptr, OID_SENSOR_SCALE_1);
    // PLL temperature compensation value
    ptr += SNMP_writeSensorScale(ptr, OID_SENSOR_SCALE_1);

    return ptr - dst;
}

int SNMP_writeSensorPrecs(uint8_t * const dst) {
    uint8_t *ptr = dst;

    // processor temperature
    ptr += SNMP_writeSensorPrec(ptr, 3);
    // PLL mean offset
    ptr += SNMP_writeSensorPrec(ptr, 1);
    // PLL RMS offset
    ptr += SNMP_writeSensorPrec(ptr, 1);
    // PLL RMS skew
    ptr += SNMP_writeSensorPrec(ptr, 1);
    // PLL correction
    ptr += SNMP_writeSensorPrec(ptr, 4);
    // PLL temperature compensation bias
    ptr += SNMP_writeSensorPrec(ptr, 4);
    // PLL temperature compensation coefficient
    ptr += SNMP_writeSensorPrec(ptr, 4);
    // PLL temperature compensation value
    ptr += SNMP_writeSensorPrec(ptr, 4);

    return ptr - dst;
}

int SNMP_writeSensorValues(uint8_t * const dst) {
    uint8_t *ptr = dst;

    // processor temperature
    ptr += SNMP_writeSensorValue(ptr, lroundf(TCMP_temp() * 1e3f));
    // PLL mean offset
    ptr += SNMP_writeSensorValue(ptr, lroundf(PLL_offsetMean() * 1e10f));
    // PLL RMS offset
    ptr += SNMP_writeSensorValue(ptr, lroundf(PLL_offsetRms() * 1e10f));
    // PLL RMS skew
    ptr += SNMP_writeSensorValue(ptr, lroundf(PLL_driftStdDev() * 1e10f));
    // PLL correction
    ptr += SNMP_writeSensorValue(ptr, lroundf(1e10f * 0x1p-32f * (float) CLK_COMP_getComp()));
    // PLL temperature compensation bias
    ptr += SNMP_writeSensorValue(ptr, lroundf(0 * 1e10f));
    // PLL temperature compensation coefficient
    ptr += SNMP_writeSensorValue(ptr, lroundf(0 * 1e10f));
    // PLL temperature compensation value
    ptr += SNMP_writeSensorValue(ptr, lroundf(TCMP_get() * 1e10f));

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
    // PLL mean offset
    ptr += SNMP_writeSensorUnit(ptr, "s");
    // PLL RMS offset
    ptr += SNMP_writeSensorUnit(ptr, "s");
    // PLL RMS skew
    ptr += SNMP_writeSensorUnit(ptr, "s");
    // PLL correction
    ptr += SNMP_writeSensorUnit(ptr, "ppm");
    // PLL temperature compensation bias
    ptr += SNMP_writeSensorUnit(ptr, "C");
    // PLL temperature compensation coefficient
    ptr += SNMP_writeSensorUnit(ptr, "ppm/C");
    // PLL temperature compensation value
    ptr += SNMP_writeSensorUnit(ptr, "ppm");

    return ptr - dst;
}

int SNMP_writeSensorUpdateTimes(uint8_t * const dst) {
    uint8_t *ptr = dst;

    uint32_t timeTicks = ((CLK_MONO() * 100) >> 32);

    // processor temperature
    ptr += SNMP_writeSensorUpdateTime(ptr, timeTicks);
    // PLL mean offset
    ptr += SNMP_writeSensorUpdateTime(ptr, timeTicks);
    // PLL RMS offset
    ptr += SNMP_writeSensorUpdateTime(ptr, timeTicks);
    // PLL RMS skew
    ptr += SNMP_writeSensorUpdateTime(ptr, timeTicks);
    // PLL correction
    ptr += SNMP_writeSensorUpdateTime(ptr, timeTicks);
    // PLL temperature compensation bias
    ptr += SNMP_writeSensorUpdateTime(ptr, timeTicks);
    // PLL temperature compensation coefficient
    ptr += SNMP_writeSensorUpdateTime(ptr, timeTicks);
    // PLL temperature compensation value
    ptr += SNMP_writeSensorUpdateTime(ptr, timeTicks);

    return ptr - dst;
}

int SNMP_writeSensorUpdateRates(uint8_t * const dst) {
    uint8_t *ptr = dst;

    // processor temperature
    ptr += SNMP_writeSensorUpdateRate(ptr, 0);
    // PLL mean offset
    ptr += SNMP_writeSensorUpdateRate(ptr, 0);
    // PLL RMS offset
    ptr += SNMP_writeSensorUpdateRate(ptr, 0);
    // PLL RMS skew
    ptr += SNMP_writeSensorUpdateRate(ptr, 0);
    // PLL correction
    ptr += SNMP_writeSensorUpdateRate(ptr, 0);
    // PLL temperature compensation bias
    ptr += SNMP_writeSensorUpdateRate(ptr, 0);
    // PLL temperature compensation coefficient
    ptr += SNMP_writeSensorUpdateRate(ptr, 0);
    // PLL temperature compensation value
    ptr += SNMP_writeSensorUpdateRate(ptr, 0);

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

int SNMP_writeSensorUpdateRate(uint8_t * const dst, int millis) {
    return SNMP_writeValueInt32(dst, OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX), OID_SENSOR_UPDATE_RATE, millis);
}
