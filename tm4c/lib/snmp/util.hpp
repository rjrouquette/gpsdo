//
// Created by robert on 5/3/23.
//

#pragma once

#include <cstdint>

namespace snmp {
    /**
     * Determine expected size of length field based of value of length
     * @param len the length value for which to determine the size
     * @return number of bytes required to encode the length value
     */
    int lengthSize(int len);

    /**
     * Determine expected size of encoded byte data based of length
     * @param len the number of bytes to be encoded
     * @return number of bytes required to encode the byte data
     */
    int lengthBytes(int len);

    /**
     * Determine expected size of encoded integer
     * @param size the size of the integer in bytes
     * @return number of bytes required to encode the integer
     */
    int lengthInteger(int size);

    int readLength(const uint8_t *data, int offset, int dlen, int &result);

    int readBytes(const uint8_t *data, int offset, int dlen, void *result, int &rlen);

    int readInt32(const uint8_t *data, int offset, int dlen, uint32_t &result);

    int readOID(const uint8_t *data, int offset, int dlen, void *result, int &rlen);

    /**
     * Write SMNP encoded length value to byte stream
     * @param dst address at which to encode data
     * @param len length value to encode
     * @return number of bytes encoded to destination
     */
    int writeLength(uint8_t *dst, int len);

    /**
     * Write SNMP encoded byte string
     * @param dst address at which to encode data
     * @param value bytes to be written
     * @param len number of bytes to be written
     * @return number of bytes encoded to destination
     */
    int writeBytes(uint8_t *dst, const void *value, int len);

    /**
     * Write SNMP encoded integer of arbitrary size
     * @param dst address at which to encode data
     * @param value pointer to integer value
     * @param size size of integer in bytes
     * @return number of bytes encoded to destination
     */
    int writeInteger(uint8_t *dst, const void *value, int size);

    int writeValueBytes(uint8_t *dst, const uint8_t *prefOID, int prefLen, uint8_t oid, const void *value, int len);

    int writeValueInt8(uint8_t *dst, const uint8_t *prefOID, int prefLen, uint8_t oid, uint8_t value);

    int writeValueInt16(uint8_t *dst, const uint8_t *prefOID, int prefLen, uint8_t oid, uint16_t value);

    int writeValueInt32(uint8_t *dst, const uint8_t *prefOID, int prefLen, uint8_t oid, uint32_t value);

    int writeValueInt64(uint8_t *dst, const uint8_t *prefOID, int prefLen, uint8_t oid, uint64_t value);

    /**
     * Wrap SNMP request variable data to from a complete response
     * @param reqId request id
     * @param dst address at which to encode data
     * @param vars variable data to wrap as a response
     * @param len length of variable data in bytes
     * @return number of bytes written to dest
     */
    int wrapVars(uint32_t reqId, uint8_t *dst, const uint8_t *vars, int len);
}
