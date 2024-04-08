//
// Created by robert on 5/3/23.
//

#include "util.hpp"
#include "../net/util.h"

#include <memory.h>


// success response header
static constexpr uint8_t RESP_HEAD[] = {0x02, 0x01, 0x00, 0x04, 0x06, 0x73, 0x74, 0x61, 0x74, 0x75, 0x73, 0xA2};
// error response header
static constexpr uint8_t RESP_ERR[] = {0x02, 0x01, 0x00, 0x02, 0x01, 0x00};


int snmp::lengthSize(const int len) {
    if (len < 128)
        return 1;
    if (len < 1 << 8)
        return 2;
    if (len < 1 << 16)
        return 3;
    if (len < 1 << 24)
        return 4;
    return 5;
}

int snmp::lengthBytes(const int len) {
    return 1 + lengthSize(len) + len;
}

int snmp::lengthInteger(const int size) {
    return 2 + size;
}

int snmp::readLength(const uint8_t *data, int offset, const int dlen, int &result) {
    if (offset < 0)
        return -1;
    if (offset >= dlen)
        return -1;
    const uint8_t head = data[offset++];
    // definite short form
    if ((head & 0x80) == 0) {
        result = head;
        return offset;
    }
    // definite long form
    int len = head & 0x7F;
    if (len == 1) {
        if ((offset + 1) >= dlen)
            return -1;
        result = data[offset++];
        return offset;
    }
    if (len == 2) {
        if ((offset + 2) >= dlen)
            return -1;
        uint16_t temp = data[offset++];
        temp <<= 8;
        temp |= data[offset++];
        result = temp;
        return offset;
    }
    return -1;
}

int snmp::readBytes(const uint8_t *data, int offset, const int dlen, void *result, int &rlen) {
    if (offset < 0)
        return -1;
    if (offset >= dlen)
        return -1;
    // verify type
    if (data[offset++] != 0x04)
        return -1;
    // get content length
    int blen;
    offset = readLength(data, offset, dlen, blen);
    if (offset < 0 || (offset + blen) > dlen)
        return -1;
    if (rlen > blen)
        rlen = blen;
    for (int i = 0; i < rlen; i++)
        static_cast<uint8_t*>(result)[i] = data[offset + i];
    return offset + blen;
}

int snmp::readInt32(const uint8_t *data, int offset, const int dlen, uint32_t &result) {
    if (offset < 0)
        return -1;
    if (offset >= dlen)
        return -1;
    uint8_t tag = data[offset++];
    // verify type
    if (tag != 0x02)
        return -1;
    // get content length
    int blen;
    offset = readLength(data, offset, dlen, blen);
    if (offset < 0 || (offset + blen) > dlen)
        return -1;
    uint32_t temp = 0;
    // odd byte sign extension
    if (blen & 1) {
        temp = static_cast<int32_t>(static_cast<int8_t>(data[offset++]));
        blen -= 1;
    }
    // even byte sign extension
    else if (blen < 4) {
        temp = static_cast<int16_t>(htons(*reinterpret_cast<const uint16_t*>(data + offset)));
        blen -= 2;
        offset += 2;
    }
    // append remaining 16-bit words
    for (int i = 0; i < blen; i += 2) {
        temp <<= 16;
        temp |= htons(*reinterpret_cast<const uint16_t*>(data + offset + i));
    }
    result = temp;
    return offset + blen;
}

int snmp::readOID(const uint8_t *data, int offset, const int dlen, void *result, int &rlen) {
    if (offset < 0)
        return -1;
    if (offset >= dlen)
        return -1;
    // verify type
    if (data[offset++] != 0x06)
        return -1;
    // get content length
    int blen;
    offset = readLength(data, offset, dlen, blen);
    if (offset < 0 || (offset + blen) > dlen)
        return -1;
    if (rlen > blen)
        rlen = blen;
    for (int i = 0; i < rlen; i++)
        static_cast<uint8_t*>(result)[i] = data[offset + i];
    return offset + blen;
}

int snmp::writeLength(uint8_t *const dst, const int len) {
    uint8_t *ptr = dst;

    if (len < 128) {
        *ptr++ = len;
        return ptr - dst;
    }
    if (len < 1 << 8) {
        *ptr++ = 0x81;
        *ptr++ = len;
        return ptr - dst;
    }
    if (len < 1 << 16) {
        *ptr++ = 0x82;
        *ptr++ = len >> 8;
        *ptr++ = len;
        return ptr - dst;
    }
    if (len < 1 << 24) {
        *ptr++ = 0x83;
        *ptr++ = len >> 16;
        *ptr++ = len >> 8;
        *ptr++ = len;
        return ptr - dst;
    }
    *ptr++ = 0x84;
    *ptr++ = len >> 24;
    *ptr++ = len >> 16;
    *ptr++ = len >> 8;
    *ptr++ = len;
    return ptr - dst;
}

int snmp::writeBytes(uint8_t *const dst, const void *value, const int len) {
    uint8_t *ptr = dst;

    *ptr++ = 0x04;
    ptr += writeLength(ptr, len);
    for (int i = 0; i < len; i++)
        ptr[i] = static_cast<const uint8_t*>(value)[i];
    return len + (ptr - dst);
}

int snmp::writeInteger(uint8_t *const dst, const void *value, int size) {
    uint8_t *ptr = dst;

    *ptr++ = 0x02;
    *ptr++ = size--;
    while (size >= 0) {
        *ptr++ = static_cast<const uint8_t*>(value)[size--];
    }
    return ptr - dst;
}

int snmp::writeValueBytes(
    uint8_t *const dst, const uint8_t *prefOID, const int prefLen, const uint8_t oid, const void *value, const int len
) {
    uint8_t *base = dst + 1 + lengthSize(lengthBytes(len));
    uint8_t *ptr = base;
    memcpy(ptr, prefOID, prefLen);
    ptr += prefLen;
    *ptr++ = oid;
    ptr += writeBytes(ptr, value, len);
    dst[0] = 0x30;
    writeLength(dst + 1, ptr - base);
    return ptr - dst;
}

int snmp::writeValueInt8(uint8_t *const dst, const uint8_t *prefOID, const int prefLen, const uint8_t oid,
                         const uint8_t value) {
    uint8_t *ptr = dst + 2;
    memcpy(ptr, prefOID, prefLen);
    ptr += prefLen;
    *ptr++ = oid;
    ptr += writeInteger(ptr, &value, sizeof(value));
    dst[0] = 0x30;
    dst[1] = ptr - dst - 2;
    return ptr - dst;
}

int snmp::writeValueInt16(uint8_t *const dst, const uint8_t *prefOID, const int prefLen, const uint8_t oid,
                          const uint16_t value) {
    uint8_t *ptr = dst + 2;
    memcpy(ptr, prefOID, prefLen);
    ptr += prefLen;
    *ptr++ = oid;
    ptr += writeInteger(ptr, &value, sizeof(value));
    dst[0] = 0x30;
    dst[1] = ptr - dst - 2;
    return ptr - dst;
}

int snmp::writeValueInt32(uint8_t *const dst, const uint8_t *prefOID, const int prefLen, const uint8_t oid,
                          const uint32_t value) {
    uint8_t *ptr = dst + 2;
    memcpy(ptr, prefOID, prefLen);
    ptr += prefLen;
    *ptr++ = oid;
    ptr += writeInteger(ptr, &value, sizeof(value));
    dst[0] = 0x30;
    dst[1] = ptr - dst - 2;
    return ptr - dst;
}

int snmp::writeValueInt64(uint8_t *const dst, const uint8_t *prefOID, const int prefLen, const uint8_t oid,
                          const uint64_t value) {
    uint8_t *ptr = dst + 2;
    memcpy(ptr, prefOID, prefLen);
    ptr += prefLen;
    *ptr++ = oid;
    ptr += writeInteger(ptr, &value, sizeof(value));
    dst[0] = 0x30;
    dst[1] = ptr - dst - 2;
    return ptr - dst;
}

int snmp::wrapVars(const uint32_t reqId, uint8_t *const dst, const uint8_t *vars, const int len) {
    int tlen = len;
    tlen += 1 + lengthSize(len);
    tlen += sizeof(RESP_ERR);
    tlen += lengthInteger(sizeof(reqId));
    int rlen = tlen;
    tlen += lengthSize(tlen);
    tlen += sizeof(RESP_HEAD);

    uint8_t *ptr = dst;
    *ptr++ = 0x30;
    ptr += writeLength(ptr, tlen);

    memcpy(ptr, RESP_HEAD, sizeof(RESP_HEAD));
    ptr += sizeof(RESP_HEAD);

    ptr += writeLength(ptr, rlen);
    ptr += writeInteger(ptr, &reqId, sizeof(reqId));

    memcpy(ptr, RESP_ERR, sizeof(RESP_ERR));
    ptr += sizeof(RESP_ERR);

    // append variable data
    *ptr++ = 0x30;
    ptr += writeLength(ptr, len);
    memcpy(ptr, vars, len);
    ptr += len;

    // return final length
    return ptr - dst;
}
