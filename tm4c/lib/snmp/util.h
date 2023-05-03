//
// Created by robert on 5/3/23.
//

#ifndef GPSDO_LIB_SNMP_UTIL_H
#define GPSDO_LIB_SNMP_UTIL_H

#include <stdint.h>

/**
 * Determine expected size of length field based of value of length
 * @param len the length value for which to determine the size
 * @return number of bytes required to encode the length value
 */
int SNMP_lengthSize(int len);

/**
 * Write SMNP encoded length value to byte stream
 * @param dst address at which to encode data
 * @param len length value to encode
 * @return number of bytes encoded to destination
 */
int SNMP_writeLength(uint8_t *dst, int len);

/**
 * Write SNMP encoded byte string
 * @param dst address at which to encode data
 * @param value bytes to be written
 * @param len number of bytes to be written
 * @return number of bytes encoded to destination
 */
int SNMP_writeBytes(uint8_t *dst, const void *value, int len);

/**
 * Write SNMP encoded integer of arbitrary size
 * @param dst address at which to encode data
 * @param value pointer to integer value
 * @param len size of integer in bytes
 * @return number of bytes encoded to destination
 */
int SNMP_writeInteger(uint8_t *dst, const void *value, int size);

int SNMP_writeValueBytes(uint8_t *dst, const uint8_t *prefOID, int prefLen, uint8_t oid, const void *value, int len);

int SNMP_writeValueInt8(uint8_t *dst, const uint8_t *prefOID, int prefLen, uint8_t oid, uint8_t value);

int SNMP_writeValueInt16(uint8_t *dst, const uint8_t *prefOID, int prefLen, uint8_t oid, uint16_t value);

int SNMP_writeValueInt32(uint8_t *dst, const uint8_t *prefOID, int prefLen, uint8_t oid, uint32_t value);

int SNMP_writeValueInt64(uint8_t *dst, const uint8_t *prefOID, int prefLen, uint8_t oid, uint64_t value);

/**
 * Wrap SNMP request variable data to from a complete response
 * @param reqId request id
 * @param dest address at which to encode data
 * @param vars variable data to wrap as a response
 * @param len length of variable data in bytes
 * @return number of bytes written to dest
 */
int SNMP_wrapVars(uint32_t reqId, uint8_t *dest, const uint8_t *vars, int len);


#endif //GPSDO_LIB_SNMP_UTIL_H
