//
// Created by robert on 5/3/23.
//

#ifndef GPSDO_LIB_SNMP_SENSORS_H
#define GPSDO_LIB_SNMP_SENSORS_H

#include <stdint.h>

#define OID_SENSOR_TYPE (1)
#define OID_SENSOR_SCALE (2)
#define OID_SENSOR_PREC (3)
#define OID_SENSOR_VALUE (4)
#define OID_SENSOR_STATUS (5)
#define OID_SENSOR_UNITS (6)
#define OID_SENSOR_UPDATE_TIME (7)
#define OID_SENSOR_UPDATE_RATE (8)

#define OID_SENSOR_TYPE_OTHER (1)
#define OID_SENSOR_TYPE_UNKNOWN (2)
#define OID_SENSOR_TYPE_VOLTS_AC (3)
#define OID_SENSOR_TYPE_VOLTS_DC (4)
#define OID_SENSOR_TYPE_AMPS (5)
#define OID_SENSOR_TYPE_WATTS (6)
#define OID_SENSOR_TYPE_HERTZ (7)
#define OID_SENSOR_TYPE_CELSIUS (8)
#define OID_SENSOR_TYPE_PERC_REL_HUM (9)
#define OID_SENSOR_TYPE_RPM (10)
#define OID_SENSOR_TYPE_CUB_MTR_MIN (11)
#define OID_SENSOR_TYPE_BOOL (12)

#define OID_SENSOR_SCALE_1E_24 (1)
#define OID_SENSOR_SCALE_1E_21 (2)
#define OID_SENSOR_SCALE_1E_18 (3)
#define OID_SENSOR_SCALE_1E_15 (4)
#define OID_SENSOR_SCALE_1E_12 (5)
#define OID_SENSOR_SCALE_1E_9 (6)
#define OID_SENSOR_SCALE_1E_6 (7)
#define OID_SENSOR_SCALE_1E_3 (8)
#define OID_SENSOR_SCALE_1 (9)
#define OID_SENSOR_SCALE_1E3 (10)
#define OID_SENSOR_SCALE_1E6 (11)
#define OID_SENSOR_SCALE_1E9 (12)
#define OID_SENSOR_SCALE_1E12 (13)
#define OID_SENSOR_SCALE_1E15 (14)
#define OID_SENSOR_SCALE_1E18 (15)
#define OID_SENSOR_SCALE_1E21 (16)
#define OID_SENSOR_SCALE_1E24 (17)

#define OID_SENSOR_STATUS_OK (1)
#define OID_SENSOR_STATUS_UNAVAILABLE (2)
#define OID_SENSOR_STATUS_NON_OP (3)

/**
 * Write SNMP response for Sensor Type OID
 * @param dst address of output buffer
 * @return number of bytes written to buffer
 */
int SNMP_writeSensorTypes(uint8_t *dst);

/**
 * Write SNMP response for Sensor Scale OID
 * @param dst address of output buffer
 * @return number of bytes written to buffer
 */
int SNMP_writeSensorScales(uint8_t *dst);

/**
 * Write SNMP response for Sensor Precision OID
 * @param dst address of output buffer
 * @return number of bytes written to buffer
 */
int SNMP_writeSensorPrecs(uint8_t *dst);

/**
 * Write SNMP response for Sensor Value OID
 * @param dst address of output buffer
 * @return number of bytes written to buffer
 */
int SNMP_writeSensorValues(uint8_t *dst);

/**
 * Write SNMP response for Sensor Status OID
 * @param dst address of output buffer
 * @return number of bytes written to buffer
 */
int SNMP_writeSensorStatuses(uint8_t *dst);

/**
 * Write SNMP response for Sensor Units OID
 * @param dst address of output buffer
 * @return number of bytes written to buffer
 */
int SNMP_writeSensorUnits(uint8_t *dst);

/**
 * Write SNMP response for Sensor Update Time OID
 * @param dst address of output buffer
 * @return number of bytes written to buffer
 */
int SNMP_writeSensorUpdateTimes(uint8_t *dst);

/**
 * Write SNMP response for Sensor Update Rate OID
 * @param dst address of output buffer
 * @return number of bytes written to buffer
 */
int SNMP_writeSensorUpdateRates(uint8_t *dst);



/**
 * Write SNMP MIB for sensor type
 * @param dst address at which to encode sensor type
 * @param typeId sensor type ID to encode
 * @return number of bytes written to destination
 */
int SNMP_writeSensorType(uint8_t *dst, int typeId);

/**
 * Write SNMP MIB for sensor scale
 * @param dst address at which to encode sensor scale
 * @param scaleId sensor scale ID to encode
 * @return number of bytes written to destination
 */
int SNMP_writeSensorScale(uint8_t *dst, int scaleId);

/**
 * Write SNMP MIB for sensor precision
 * @param dst address at which to encode sensor precision
 * @param precision sensor precision divisor (log10)
 * @return number of bytes written to destination
 */
int SNMP_writeSensorPrec(uint8_t *dst, int precision);

/**
 * Write SNMP MIB for sensor value
 * @param dst address at which to encode sensor value
 * @param precision sensor value scaled relative to precision
 * @return number of bytes written to destination
 */
int SNMP_writeSensorValue(uint8_t *dst, int value);

/**
 * Write SNMP MIB for sensor precision
 * @param dst address at which to encode sensor precision
 * @param precision sensor precision divisor (log10)
 * @return number of bytes written to destination
 */
int SNMP_writeSensorStatus(uint8_t *dst, int statusId);

/**
 * Write SNMP MIB for sensor unit
 * @param dst address at which to encode sensor unit
 * @param unit string representation of senor unit
 * @return number of bytes written to destination
 */
int SNMP_writeSensorUnit(uint8_t *dst, const char *unit);

/**
 * Write SNMP MIB for sensor update time
 * @param dst address at which to encode sensor update time
 * @param timeTicks timestamp of sensor update (SNMP TimeTick Format)
 * @return number of bytes written to destination
 */
int SNMP_writeSensorUpdateTime(uint8_t *dst, uint32_t timeTicks);

/**
 * Write SNMP MIB for sensor update rate
 * @param dst address at which to encode sensor update rate
 * @param millis update rate in milliseconds
 * @return number of bytes written to destination
 */
int SNMP_writeSensorUpdateRate(uint8_t *dst, int millis);

#endif //GPSDO_LIB_SNMP_SENSORS_H
