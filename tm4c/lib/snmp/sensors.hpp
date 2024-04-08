//
// Created by robert on 5/3/23.
//

#pragma once

#include <cstdint>

namespace snmp {
    static constexpr int OID_SENSOR_TYPE = 1;
    static constexpr int OID_SENSOR_SCALE = 2;
    static constexpr int OID_SENSOR_PREC = 3;
    static constexpr int OID_SENSOR_VALUE = 4;
    static constexpr int OID_SENSOR_STATUS = 5;
    static constexpr int OID_SENSOR_UNITS = 6;
    static constexpr int OID_SENSOR_UPDATE_TIME = 7;
    static constexpr int OID_SENSOR_UPDATE_RATE = 8;
    static constexpr int OID_SENSOR_NAME = 9; // not part of the RFC

    /**
     * Write SNMP response for Sensor Type OID
     * @param dst address of output buffer
     * @return number of bytes written to buffer
     */
    int writeSensorTypes(uint8_t *dst);

    /**
     * Write SNMP response for Sensor Scale OID
     * @param dst address of output buffer
     * @return number of bytes written to buffer
     */
    int writeSensorScales(uint8_t *dst);

    /**
     * Write SNMP response for Sensor Precision OID
     * @param dst address of output buffer
     * @return number of bytes written to buffer
     */
    int writeSensorPrecs(uint8_t *dst);

    /**
     * Write SNMP response for Sensor Value OID
     * @param dst address of output buffer
     * @return number of bytes written to buffer
     */
    int writeSensorValues(uint8_t *dst);

    /**
     * Write SNMP response for Sensor Status OID
     * @param dst address of output buffer
     * @return number of bytes written to buffer
     */
    int writeSensorStatuses(uint8_t *dst);

    /**
     * Write SNMP response for Sensor Units OID
     * @param dst address of output buffer
     * @return number of bytes written to buffer
     */
    int writeSensorUnits(uint8_t *dst);

    /**
     * Write SNMP response for Sensor Update Time OID
     * @param dst address of output buffer
     * @return number of bytes written to buffer
     */
    int writeSensorUpdateTimes(uint8_t *dst);

    /**
     * Write SNMP response for Sensor Update Rate OID
     * @param dst address of output buffer
     * @return number of bytes written to buffer
     */
    int writeSensorUpdateRates(uint8_t *dst);

    /**
     * Write SNMP response for Sensor Name OID
     * @param dst address of output buffer
     * @return number of bytes written to buffer
     */
    int writeSensorNames(uint8_t *dst);
}
