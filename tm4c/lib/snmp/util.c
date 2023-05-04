//
// Created by robert on 5/3/23.
//

#include <memory.h>
#include "util.h"


// success response header
static const uint8_t RESP_HEAD[] = { 0x02, 0x01, 0x00, 0x04, 0x06, 0x73, 0x74, 0x61, 0x74, 0x75, 0x73, 0xA2 };
// error response header
static const uint8_t RESP_ERR[] = { 0x02, 0x01, 0x00, 0x02, 0x01, 0x00 };



int SNMP_lengthSize(int len) {
    if(len < 128)
        return 1;
    if(len < (1<<8))
        return 2;
    if(len < (1<<16))
        return 3;
    if(len < (1<<24))
        return 4;
    return 5;
}

int SNMP_lengthBytes(int len) {
    return 1 + SNMP_lengthSize(len) + len;
}

int SNMP_lengthInteger(int size) {
    return 2 + size;
}

int SNMP_writeLength(uint8_t * const dst, int len) {
    uint8_t *ptr = dst;

    if(len < 128) {
        *(ptr++) = len;
        return ptr - dst;
    }
    if(len < (1<<8)) {
        *(ptr++) = 0x81;
        *(ptr++) = len;
        return ptr - dst;
    }
    if(len < (1<<16)) {
        *(ptr++) = 0x82;
        *(ptr++) = len >> 8;
        *(ptr++) = len;
        return ptr - dst;
    }
    if(len < (1<<24)) {
        *(ptr++) = 0x83;
        *(ptr++) = len >> 16;
        *(ptr++) = len >> 8;
        *(ptr++) = len;
        return ptr - dst;
    }
    *(ptr++) = 0x84;
    *(ptr++) = len >> 24;
    *(ptr++) = len >> 16;
    *(ptr++) = len >> 8;
    *(ptr++) = len;
    return ptr - dst;
}

int SNMP_writeBytes(uint8_t * const dst, const void *value, int len) {
    uint8_t *ptr = dst;

    *(ptr++) = 0x04;
    ptr += SNMP_writeLength(ptr, len);
    for(int i = 0; i < len; i++)
        ptr[i] = ((uint8_t *) value)[i];
    return len + (ptr - dst);
}

int SNMP_writeInteger(uint8_t * const dst, const void *value, int size) {
    uint8_t *ptr = dst;

    *(ptr++) = 0x02;
    *(ptr++) = size--;
    while(size >= 0) {
        *(ptr++) = ((uint8_t *) value)[size--];
    }
    return ptr - dst;
}

int SNMP_writeValueBytes(
        uint8_t * const dst, const uint8_t *prefOID, int prefLen, uint8_t oid, const void *value, int len
) {
    uint8_t *base = dst + 1 + SNMP_lengthSize(SNMP_lengthBytes(len));
    uint8_t *ptr = base;
    memcpy(ptr, prefOID, prefLen);
    ptr += prefLen;
    *(ptr++) = oid;
    ptr += SNMP_writeBytes(ptr, value, len);
    dst[0] = 0x30;
    SNMP_writeLength(dst + 1, ptr - base);
    return ptr - dst;
}

int SNMP_writeValueInt8(uint8_t * const dst, const uint8_t *prefOID, int prefLen, uint8_t oid, uint8_t value) {
    uint8_t *ptr = dst + 2;
    memcpy(ptr, prefOID, prefLen);
    ptr += prefLen;
    *(ptr++) = oid;
    ptr += SNMP_writeInteger(ptr, &value, sizeof(value));
    dst[0] = 0x30;
    dst[1] = (ptr - dst) - 2;
    return ptr - dst;
}

int SNMP_writeValueInt16(uint8_t * const dst, const uint8_t *prefOID, int prefLen, uint8_t oid, uint16_t value) {
    uint8_t *ptr = dst + 2;
    memcpy(ptr, prefOID, prefLen);
    ptr += prefLen;
    *(ptr++) = oid;
    ptr += SNMP_writeInteger(ptr, &value, sizeof(value));
    dst[0] = 0x30;
    dst[1] = (ptr - dst) - 2;
    return ptr - dst;
}

int SNMP_writeValueInt32(uint8_t * const dst, const uint8_t *prefOID, int prefLen, uint8_t oid, uint32_t value) {
    uint8_t *ptr = dst + 2;
    memcpy(ptr, prefOID, prefLen);
    ptr += prefLen;
    *(ptr++) = oid;
    ptr += SNMP_writeInteger(ptr, &value, sizeof(value));
    dst[0] = 0x30;
    dst[1] = (ptr - dst) - 2;
    return ptr - dst;
}

int SNMP_writeValueInt64(uint8_t * const dst, const uint8_t *prefOID, int prefLen, uint8_t oid, uint64_t value) {
    uint8_t *ptr = dst + 2;
    memcpy(ptr, prefOID, prefLen);
    ptr += prefLen;
    *(ptr++) = oid;
    ptr += SNMP_writeInteger(ptr, &value, sizeof(value));
    dst[0] = 0x30;
    dst[1] = (ptr - dst) - 2;
    return ptr - dst;
}

int SNMP_wrapVars(const uint32_t reqId, uint8_t * const dst, const uint8_t *vars, int len) {
    int tlen = len;
    tlen += 1 + SNMP_lengthSize(len);
    tlen += sizeof(RESP_ERR);
    tlen += SNMP_lengthInteger(sizeof(reqId));
    int rlen = tlen;
    tlen += SNMP_lengthSize(tlen);
    tlen += sizeof(RESP_HEAD);

    uint8_t *ptr = dst;
    *(ptr++) = 0x30;
    ptr += SNMP_writeLength(ptr, tlen);

    memcpy(ptr, RESP_HEAD, sizeof(RESP_HEAD));
    ptr += sizeof(RESP_HEAD);

    ptr += SNMP_writeLength(ptr, rlen);
    ptr += SNMP_writeInteger(ptr, &reqId, sizeof(reqId));

    memcpy(ptr, RESP_ERR, sizeof(RESP_ERR));
    ptr += sizeof(RESP_ERR);

    // append variable data
    *(ptr++) = 0x30;
    ptr += SNMP_writeLength(dst, len);
    memcpy(ptr, vars, len);
    ptr += len;

    // return final length
    return ptr - dst;
}
