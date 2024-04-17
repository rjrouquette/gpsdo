//
// Created by robert on 5/3/23.
//

#include "sensors.hpp"

#include "util.hpp"
#include "../gps.hpp"
#include "../clock/mono.hpp"
#include "../ntp/pll.hpp"
#include "../ntp/tcmp.hpp"

#include <cmath>
#include <cstring>
#include <iterator>


static constexpr int OID_SENSOR_TYPE_OTHER = 1;
// static constexpr int OID_SENSOR_TYPE_UNKNOWN = 2;
// static constexpr int OID_SENSOR_TYPE_VOLTS_AC = 3;
// static constexpr int OID_SENSOR_TYPE_VOLTS_DC = 4;
// static constexpr int OID_SENSOR_TYPE_AMPS = 5;
// static constexpr int OID_SENSOR_TYPE_WATTS = 6;
// static constexpr int OID_SENSOR_TYPE_HERTZ = 7;
static constexpr int OID_SENSOR_TYPE_CELSIUS = 8;
// static constexpr int OID_SENSOR_TYPE_PERC_REL_HUM = 9;
// static constexpr int OID_SENSOR_TYPE_RPM = 10;
// static constexpr int OID_SENSOR_TYPE_CUB_MTR_MIN = 11;
static constexpr int OID_SENSOR_TYPE_BOOL = 12;

// static constexpr int OID_SENSOR_SCALE_1E_24 = 1;
// static constexpr int OID_SENSOR_SCALE_1E_21 = 2;
// static constexpr int OID_SENSOR_SCALE_1E_18 = 3;
// static constexpr int OID_SENSOR_SCALE_1E_15 = 4;
static constexpr int OID_SENSOR_SCALE_1E_12 = 5;
static constexpr int OID_SENSOR_SCALE_1E_9 = 6;
static constexpr int OID_SENSOR_SCALE_1E_6 = 7;
// static constexpr int OID_SENSOR_SCALE_1E_3 = 8;
static constexpr int OID_SENSOR_SCALE_1 = 9;
// static constexpr int OID_SENSOR_SCALE_1E3 = 10;
// static constexpr int OID_SENSOR_SCALE_1E6 = 11;
// static constexpr int OID_SENSOR_SCALE_1E9 = 12;
// static constexpr int OID_SENSOR_SCALE_1E12 = 13;
// static constexpr int OID_SENSOR_SCALE_1E15 = 14;
// static constexpr int OID_SENSOR_SCALE_1E18 = 15;
// static constexpr int OID_SENSOR_SCALE_1E21 = 16;
// static constexpr int OID_SENSOR_SCALE_1E24 = 17;

static constexpr int OID_SENSOR_STATUS_OK = 1;
// static constexpr int OID_SENSOR_STATUS_UNAVAILABLE = 2;
// static constexpr int OID_SENSOR_STATUS_NON_OP = 3;

static constexpr uint8_t OID_SENSOR_PREFIX[] = {0x06, 0x0A, 0x2B, 6, 1, 2, 1, 99, 1, 1, 1};


// CPU temperature getter
static int getCpuTemp() {
    return lroundf(tcmp::temp() * 1e4f);
}

// GPS timing status getters
static int getGpsHasFix() {
    return gps::hasFix() ? 1 : 2;
}

// PLL offset getters
static int getPllOffsetLast() {
    return lroundf(PLL_offsetLast() * 1e10f);
}

static int getPllOffsetMean() {
    return lroundf(PLL_offsetMean() * 1e10f);
}

static int getPllOffsetRms() {
    return lroundf(PLL_offsetRms() * 1e10f);
}

static int getPllOffsetStdDev() {
    return lroundf(PLL_offsetStdDev() * 1e10f);
}

static int getPllOffsetProp() {
    return lroundf(PLL_offsetProp() * 1e10f);
}

static int getPllOffsetInt() {
    return lroundf(PLL_offsetInt() * 1e10f);
}

static int getPllOffsetCorr() {
    return lroundf(PLL_offsetCorr() * 1e10f);
}

// PLL drift getters
static int getPllDriftLast() {
    return lroundf(PLL_driftLast() * 1e10f);
}

static int getPllDriftMean() {
    return lroundf(PLL_driftMean() * 1e10f);
}

static int getPllDriftRms() {
    return lroundf(PLL_driftRms() * 1e10f);
}

static int getPllDriftStdDev() {
    return lroundf(PLL_driftStdDev() * 1e10f);
}

static int getPllDriftCorr() {
    return lroundf(PLL_driftCorr() * 1e10f);
}

static int getPllDriftFreq() {
    return lroundf(PLL_driftFreq() * 1e10f);
}


// SNMP Sensor Registry
static constexpr struct SnmpSensor {
    const char *name;
    const char *units;
    uint8_t typeId;
    uint8_t scale;
    uint8_t precision;
    int (*getter)();
} snmpSensors[] = {
    // CPU temperature
    {"cpu.temp", "C", OID_SENSOR_TYPE_CELSIUS, OID_SENSOR_SCALE_1, 4, getCpuTemp},
    // GPS timing status
    {"gps.hasFix", "", OID_SENSOR_TYPE_BOOL, OID_SENSOR_SCALE_1, 0, getGpsHasFix},
    {"gps.taiOffset", "s", OID_SENSOR_TYPE_OTHER, OID_SENSOR_SCALE_1, 0, gps::taiOffset},
    {"gps.accuracy.time", "s", OID_SENSOR_TYPE_OTHER, OID_SENSOR_SCALE_1E_9, 0, gps::accTime},
    {"gps.accuracy.freq", "s/s", OID_SENSOR_TYPE_OTHER, OID_SENSOR_SCALE_1E_12, 0, gps::accFreq},
    // PLL offset stats
    {"pll.offset.last", "s", OID_SENSOR_TYPE_OTHER, OID_SENSOR_SCALE_1E_6, 4, getPllOffsetLast},
    {"pll.offset.mean", "s", OID_SENSOR_TYPE_OTHER, OID_SENSOR_SCALE_1E_6, 4, getPllOffsetMean},
    {"pll.offset.rms", "s", OID_SENSOR_TYPE_OTHER, OID_SENSOR_SCALE_1E_6, 4, getPllOffsetRms},
    {"pll.offset.stddev", "s", OID_SENSOR_TYPE_OTHER, OID_SENSOR_SCALE_1E_6, 4, getPllOffsetStdDev},
    {"pll.offset.prop", "s/s", OID_SENSOR_TYPE_OTHER, OID_SENSOR_SCALE_1E_6, 4, getPllOffsetProp},
    {"pll.offset.int", "s/s", OID_SENSOR_TYPE_OTHER, OID_SENSOR_SCALE_1E_6, 4, getPllOffsetInt},
    {"pll.offset.corr", "s/s", OID_SENSOR_TYPE_OTHER, OID_SENSOR_SCALE_1E_6, 4, getPllOffsetCorr},
    // PLL drift stats
    {"pll.drift.last", "s/s", OID_SENSOR_TYPE_OTHER, OID_SENSOR_SCALE_1E_6, 4, getPllDriftLast},
    {"pll.drift.mean", "s/s", OID_SENSOR_TYPE_OTHER, OID_SENSOR_SCALE_1E_6, 4, getPllDriftMean},
    {"pll.drift.rms", "s/s", OID_SENSOR_TYPE_OTHER, OID_SENSOR_SCALE_1E_6, 4, getPllDriftRms},
    {"pll.drift.stddev", "s/s", OID_SENSOR_TYPE_OTHER, OID_SENSOR_SCALE_1E_6, 4, getPllDriftStdDev},
    {"pll.drift.corr", "s/s", OID_SENSOR_TYPE_OTHER, OID_SENSOR_SCALE_1E_6, 4, getPllDriftCorr},
    {"pll.drift.freq", "s/s", OID_SENSOR_TYPE_OTHER, OID_SENSOR_SCALE_1E_6, 4, getPllDriftFreq}
};

constexpr int SENSOR_CNT = std::size(snmpSensors);

int snmp::writeSensorTypes(uint8_t *const dst) {
    uint8_t *ptr = dst;

    // report sensor type
    for (const auto &sensor : snmpSensors) {
        ptr += snmp::writeValueInt8(
            ptr,
            OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX),
            OID_SENSOR_TYPE, sensor.typeId
        );
    }

    return ptr - dst;
}

int snmp::writeSensorScales(uint8_t *const dst) {
    uint8_t *ptr = dst;

    // report sensor scale
    for (const auto &sensor : snmpSensors) {
        ptr += snmp::writeValueInt8(
            ptr,
            OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX),
            OID_SENSOR_SCALE, sensor.scale
        );
    }

    return ptr - dst;
}

int snmp::writeSensorPrecs(uint8_t *const dst) {
    uint8_t *ptr = dst;

    // report sensor precision
    for (const auto &sensor : snmpSensors) {
        ptr += snmp::writeValueInt8(
            ptr,
            OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX),
            OID_SENSOR_PREC, sensor.precision
        );
    }

    return ptr - dst;
}

int snmp::writeSensorValues(uint8_t *const dst) {
    uint8_t *ptr = dst;

    // report sensor values
    for (const auto &sensor : snmpSensors) {
        ptr += snmp::writeValueInt32(
            ptr,
            OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX),
            OID_SENSOR_VALUE, (*sensor.getter)()
        );
    }

    return ptr - dst;
}

int snmp::writeSensorStatuses(uint8_t *const dst) {
    uint8_t *ptr = dst;

    // sensors are virtual and  always present
    for (int i = 0; i < SENSOR_CNT; ++i) {
        ptr += snmp::writeValueInt8(
            ptr,
            OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX),
            OID_SENSOR_STATUS, OID_SENSOR_STATUS_OK
        );
    }

    return ptr - dst;
}

int snmp::writeSensorUnits(uint8_t *const dst) {
    uint8_t *ptr = dst;

    // report human-readable units
    for (const auto &sensor : snmpSensors) {
        const char *units = sensor.units;
        ptr += snmp::writeValueBytes(
            ptr,
            OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX),
            OID_SENSOR_UNITS, units, static_cast<int>(strlen(units))
        );
    }

    return ptr - dst;
}

int snmp::writeSensorUpdateTimes(uint8_t *const dst) {
    uint8_t *ptr = dst;

    // sensors are virtual and always current
    const uint32_t timeTicks = ((clock::monotonic::now() * 100) >> 32);
    for (int i = 0; i < SENSOR_CNT; ++i) {
        ptr += snmp::writeValueInt32(
            ptr,
            OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX),
            OID_SENSOR_UPDATE_TIME, timeTicks
        );
    }

    return ptr - dst;
}

int snmp::writeSensorUpdateRates(uint8_t *const dst) {
    uint8_t *ptr = dst;

    // sensors are virtual and always current
    for (int i = 0; i < SENSOR_CNT; ++i) {
        ptr += snmp::writeValueInt8(
            ptr,
            OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX),
            OID_SENSOR_UPDATE_RATE, 0
        );
    }

    return ptr - dst;
}

int snmp::writeSensorNames(uint8_t *const dst) {
    uint8_t *ptr = dst;

    // report human-readable names
    for (const auto &sensor : snmpSensors) {
        const char *name = sensor.name;
        ptr += snmp::writeValueBytes(
            ptr,
            OID_SENSOR_PREFIX, sizeof(OID_SENSOR_PREFIX),
            OID_SENSOR_NAME, name, static_cast<int>(strlen(name))
        );
    }

    return ptr - dst;
}
