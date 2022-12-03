//
// Created by robert on 12/2/22.
//

#include <string.h>

#include "status.h"
#include "lib/clk.h"
#include "lib/format.h"
#include "lib/led.h"
#include "lib/net.h"
#include "lib/net/eth.h"
#include "lib/net/ip.h"
#include "lib/net/udp.h"
#include "lib/net/util.h"
#include "gpsdo.h"
#include "hw/emac.h"
#include "lib/net/dhcp.h"
#include "hw/sys.h"

#define STATUS_PORT (23) // telnet port

void STATUS_process(uint8_t *frame, int flen);

unsigned statusETH(char *body);
unsigned statusGPS(char *body);
unsigned statusGPSDO(char *body);
unsigned statusNTP(char *body);
unsigned statusSystem(char *body);

int hasTerminus(const char *str, int offset) {
    if(str[offset] == 0) return 1;
    if(str[offset] == '\n') return 1;
    if(str[offset] == '\r') return 1;
    return 0;
}

void STATUS_init() {
    UDP_register(STATUS_PORT, STATUS_process);
}

void STATUS_process(uint8_t *frame, int flen) {
    // restrict length
    if(flen > 128) return;
    // map headers
    struct FRAME_ETH *headerEth = (struct FRAME_ETH *) frame;
    struct HEADER_IPv4 *headerIPv4 = (struct HEADER_IPv4 *) (headerEth + 1);
    struct HEADER_UDP *headerUDP = (struct HEADER_UDP *) (headerIPv4 + 1);
    // verify destination
    if(headerIPv4->dst != ipAddress) return;
    // restrict length
    unsigned size = __builtin_bswap16(headerUDP->length) - sizeof(struct HEADER_UDP);
    if(size > 31) return;
    // status activity
    LED_act1();

    // modify ethernet frame header
    copyMAC(headerEth->macDst, headerEth->macSrc);
    // modify IP header
    headerIPv4->dst = headerIPv4->src;
    headerIPv4->src = ipAddress;
    // modify UDP header
    headerUDP->portDst = headerUDP->portSrc;
    headerUDP->portSrc = __builtin_bswap16(STATUS_PORT);

    // get packet body
    char *body = (char *) (headerUDP + 1);
    // force null termination
    body[size] = 0;
    if(strncmp(body, "ethernet", 8) == 0 && hasTerminus(body, 8)) {
        size = statusETH(body);
    } else if(strncmp(body, "gps", 3) == 0 && hasTerminus(body, 3)) {
        size = statusGPS(body);
    } else if(strncmp(body, "gpsdo", 5) == 0 && hasTerminus(body, 5)) {
        size = statusGPSDO(body);
    } else if(strncmp(body, "ntp", 3) == 0 && hasTerminus(body, 3)) {
        size = statusNTP(body);
    }  else if(strncmp(body, "system", 6) == 0 && hasTerminus(body, 6)) {
        size = statusSystem(body);
    } else {
        char tmp[32];
        strncpy(tmp, body, size);
        char *end = body;
        end = append(end, "invalid command: ");
        end = append(end, tmp);
        end = append(end, "\n");
        size = end - body;
    }

    // finalize response
    flen = UDP_DATA_OFFSET + size;
    UDP_finalize(frame, flen);
    IPv4_finalize(frame, flen);
    // transmit response
    int txDesc = NET_getTxDesc();
    if(txDesc < 0) return;
    memcpy(NET_getTxBuff(txDesc), frame, flen);
    NET_transmit(txDesc, flen);
}

unsigned statusETH(char *body) {
    char tmp[32];
    char *end = body;

    // dhcp hostname
    end = append(end, "hostname: ");
    end = append(end, DHCP_hostname());
    end = append(end, "\n");

    // mac address
    NET_getMacAddress(tmp);
    end = append(end, "mac address: ");
    end = append(end, tmp);
    end = append(end, "\n");

    // ip address
    NET_getIpAddress(tmp);
    end = append(end, "ip address: ");
    end = append(end, tmp);
    end = append(end, "\n");

    // rx packets
    tmp[toBase(EMAC0.RXCNTGB, 10, tmp)] = 0;
    end = append(end, "rx packets: ");
    end = append(end, tmp);
    end = append(end, "\n");

    // tx packets
    tmp[toBase(EMAC0.TXCNTGB, 10, tmp)] = 0;
    end = append(end, "tx packets: ");
    end = append(end, tmp);
    end = append(end, "\n");

    // link status
    int phyStatus = NET_getPhyStatus();
    end = append(end, "link speed: ");
    end = append(end, (phyStatus & PHY_STATUS_100M) ? "100M " : "10M ");
    end = append(end, (phyStatus & PHY_STATUS_DUPLEX) ? "FDX" : "HDX");
    end = append(end, "\n");

    // return size
    return end - body;
}

unsigned statusGPS(char *body) {
    return 0;
}

unsigned statusGPSDO(char *body) {
    char tmp[32];
    char *end = body;

    // pll lock
    end = append(end, "pll locked: ");
    end = append(end, GPSDO_isLocked() ? "yes" : "no");
    end = append(end, "\n");

    // current offset
    tmp[fmtFloat((float) GPSDO_offsetNano(), 0, 0, tmp)] = 0;
    end = append(end, "current offset: ");
    end = append(end, tmp);
    end = append(end, " ns\n");

    // mean offset
    tmp[fmtFloat(GPSDO_offsetMean() * 1e9f, 0, 1, tmp)] = 0;
    end = append(end, "mean offset: ");
    end = append(end, tmp);
    end = append(end, " ns\n");

    // rms offset
    tmp[fmtFloat(GPSDO_offsetRms() * 1e9f, 0, 1, tmp)] = 0;
    end = append(end, "rms offset: ");
    end = append(end, tmp);
    end = append(end, " ns\n");

    // frequency skew
    tmp[fmtFloat(GPSDO_skewRms() * 1e6f, 0, 4, tmp)] = 0;
    end = append(end, "frequency skew: ");
    end = append(end, tmp);
    end = append(end, " ppm\n");

    // frequency correction
    tmp[fmtFloat(GPSDO_freqCorr() * 1e6f, 0, 4, tmp)] = 0;
    end = append(end, "frequency correction: ");
    end = append(end, tmp);
    end = append(end, " ppm\n");

    // temperature compensation
    tmp[fmtFloat(GPSDO_compValue() * 1e6f, 0, 4, tmp)] = 0;
    end = append(end, "temperature compensation: ");
    end = append(end, tmp);
    end = append(end, " ppm\n");

    // temperature coefficient
    tmp[fmtFloat(GPSDO_compCoeff() * 1e6f, 0, 4, tmp)] = 0;
    end = append(end, "temperature coefficient: ");
    end = append(end, tmp);
    end = append(end, " ppm/C\n");

    // temperature offset
    tmp[fmtFloat(GPSDO_compBias() * 1e6f, 0, 4, tmp)] = 0;
    end = append(end, "temperature offset: ");
    end = append(end, tmp);
    end = append(end, " ppm\n");

    // return size
    return end - body;
}

unsigned statusNTP(char *body) {
    return 0;
}

unsigned statusSystem(char *body) {
    char tmp[64];
    char *end = body;

    // mcu devide id
    end = append(end, "device id: 0x");
    toHex(DID0.raw, 8, '0', tmp);
    tmp[8] = 0;
    end = append(end, " 0x");
    toHex(DID1.raw, 8, '0', tmp);
    tmp[8] = 0;
    end = append(end, tmp);
    end = append(end, "\n");

    // mcu unique id
    toHex(UNIQUEID.WORD[3], 8, '0', tmp + 0);
    toHex(UNIQUEID.WORD[2], 8, '0', tmp + 4);
    toHex(UNIQUEID.WORD[1], 8, '0', tmp + 8);
    toHex(UNIQUEID.WORD[0], 8, '0', tmp + 12);
    tmp[32] = 0;
    end = append(end, "unique id: 0x");
    end = append(end, tmp);
    end = append(end, "\n");

    // RAM size
    tmp[toBase(RAM_size(), 10, tmp)] = 0;
    end = append(end, "ram size: ");
    end = append(end, tmp);
    end = append(end, "\n");

    // EEPROM size
    tmp[toBase(EEPROM_size(), 10, tmp)] = 0;
    end = append(end, "eeprom size: ");
    end = append(end, tmp);
    end = append(end, "\n");

    // FLASH size
    tmp[toBase(FLASH_size(), 10, tmp)] = 0;
    end = append(end, "flash size: ");
    end = append(end, tmp);
    end = append(end, "\n");

    // return size
    return end - body;
}
