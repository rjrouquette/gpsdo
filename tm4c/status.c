//
// Created by robert on 12/2/22.
//

#include <string.h>

#include "hw/eeprom.h"
#include "hw/emac.h"
#include "hw/sys.h"
#include "lib/clk/clk.h"
#include "lib/clk/mono.h"
#include "lib/clk/tai.h"
#include "lib/clk/comp.h"
#include "lib/format.h"
#include "lib/gps.h"
#include "lib/led.h"
#include "lib/net.h"
#include "lib/net/arp.h"
#include "lib/net/dhcp.h"
#include "lib/net/eth.h"
#include "lib/net/ip.h"
#include "lib/net/udp.h"
#include "lib/net/util.h"
#include "lib/ntp/pll.h"

#include "gitversion.h"
#include "status.h"
#include "lib/run.h"

#define STATUS_PORT (23) // telnet port

void STATUS_process(uint8_t *frame, int flen);

unsigned statusClock(char *body);
unsigned statusEEPROM(int block, char *body);
unsigned statusETH(char *body);
unsigned statusGPS(char *body);
unsigned statusSystem(char *body);

unsigned statusSom(char *buffer);

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
    if(strncmp(body, "clock", 5) == 0 && hasTerminus(body, 8)) {
        size = statusClock(body);
    } else if(strncmp(body, "eeprom", 6) == 0 && hasTerminus(body, 8)) {
        size = statusEEPROM((int) fromHex(body + 6, 2), body);
    } else if(strncmp(body, "ethernet", 8) == 0 && hasTerminus(body, 8)) {
        size = statusETH(body);
    } else if(strncmp(body, "gps", 3) == 0 && hasTerminus(body, 3)) {
        size = statusGPS(body);
    } else if(strncmp(body, "pll", 3) == 0 && hasTerminus(body, 3)) {
        size = PLL_status(body);
    } else if(strncmp(body, "system", 6) == 0 && hasTerminus(body, 6)) {
        size = statusSystem(body);
    } else if(strncmp(body, "run", 3) == 0 && hasTerminus(body, 3)) {
        size = runStatus(body);
    }  else if(strncmp(body, "som", 3) == 0 && hasTerminus(body, 3)) {
        size = statusSom(body);
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

uint64_t tsDebug[3];

unsigned statusClock(char *body) {
    char tmp[32];
    char *end = body;

    // current time
    uint64_t mono = CLK_MONO();
    strcpy(tmp, "0x");
    toHex(mono>>32, 8, '0', tmp+2);
    tmp[10] = '.';
    toHex(mono, 8, '0', tmp+11);
    tmp[19] = 0;
    end = append(end, "clk mono:  ");
    end = append(end, tmp);
    end = append(end, "\n");

    // current time
    uint64_t comp = CLK_COMP();
    strcpy(tmp, "0x");
    toHex(comp>>32, 8, '0', tmp+2);
    tmp[10] = '.';
    toHex(comp, 8, '0', tmp+11);
    tmp[19] = 0;
    end = append(end, "clk comp:  ");
    end = append(end, tmp);
    end = append(end, "\n");

    // TAI time
    uint64_t tai = CLK_TAI();
    strcpy(tmp, "0x");
    toHex(tai>>32, 8, '0', tmp+2);
    tmp[10] = '.';
    toHex(tai, 8, '0', tmp+11);
    tmp[19] = 0;
    end = append(end, "clk tai:   ");
    end = append(end, tmp);
    end = append(end, "\n\n");

    // current time
    strcpy(tmp, "0x");
    toHex(clkCompRate, 8, '0', tmp + 2);
    tmp[10] = 0;
    end = append(end, "comp rate: ");
    end = append(end, tmp);
    end = append(end, "\n");

    // current time
    strcpy(tmp, "0x");
    toHex(clkCompRef >> 32, 8, '0', tmp + 2);
    tmp[10] = '.';
    toHex(clkCompRef, 8, '0', tmp + 11);
    tmp[19] = 0;
    end = append(end, "comp ref:  ");
    end = append(end, tmp);
    end = append(end, "\n");

    // current time
    strcpy(tmp, "0x");
    toHex(clkCompOffset >> 32, 8, '0', tmp + 2);
    tmp[10] = '.';
    toHex(clkCompOffset, 8, '0', tmp + 11);
    tmp[19] = 0;
    end = append(end, "comp off:  ");
    end = append(end, tmp);
    end = append(end, "\n\n");

    // current time
    strcpy(tmp, "0x");
    toHex(clkTaiRate, 8, '0', tmp + 2);
    tmp[10] = 0;
    end = append(end, "tai rate:  ");
    end = append(end, tmp);
    end = append(end, "\n");

    // current time
    strcpy(tmp, "0x");
    toHex(clkTaiRef >> 32, 8, '0', tmp + 2);
    tmp[10] = '.';
    toHex(clkTaiRef, 8, '0', tmp + 11);
    tmp[19] = 0;
    end = append(end, "tai ref:   ");
    end = append(end, tmp);
    end = append(end, "\n");

    // current time
    strcpy(tmp, "0x");
    toHex(clkTaiOffset >> 32, 8, '0', tmp + 2);
    tmp[10] = '.';
    toHex(clkTaiOffset, 8, '0', tmp + 11);
    tmp[19] = 0;
    end = append(end, "tai off:   ");
    end = append(end, tmp);
    end = append(end, "\n\n");

    // PPS status
    uint64_t pps[3];
    CLK_PPS(pps);
    strcpy(tmp, "0x");
    toHex(pps[0]>>32, 8, '0', tmp+2);
    tmp[10] = '.';
    toHex(pps[0], 8, '0', tmp+11);
    tmp[19] = 0;
    end = append(end, "pps mono:  ");
    end = append(end, tmp);
    end = append(end, "\n");

    strcpy(tmp, "0x");
    toHex(pps[1]>>32, 8, '0', tmp+2);
    tmp[10] = '.';
    toHex(pps[1], 8, '0', tmp+11);
    tmp[19] = 0;
    end = append(end, "pps comp:  ");
    end = append(end, tmp);
    end = append(end, "\n");

    strcpy(tmp, "0x");
    toHex(pps[2]>>32, 8, '0', tmp+2);
    tmp[10] = '.';
    toHex(pps[2], 8, '0', tmp+11);
    tmp[19] = 0;
    end = append(end, "pps tai:   ");
    end = append(end, tmp);
    end = append(end, "\n\n");

    // debug status
    strcpy(tmp, "0x");
    toHex(tsDebug[0]>>32, 8, '0', tmp+2);
    tmp[10] = '.';
    toHex(tsDebug[0], 8, '0', tmp+11);
    tmp[19] = 0;
    end = append(end, "dbg mono:  ");
    end = append(end, tmp);
    end = append(end, "\n");

    strcpy(tmp, "0x");
    toHex(tsDebug[1]>>32, 8, '0', tmp+2);
    tmp[10] = '.';
    toHex(tsDebug[1], 8, '0', tmp+11);
    tmp[19] = 0;
    end = append(end, "dbg comp:  ");
    end = append(end, tmp);
    end = append(end, "\n");

    strcpy(tmp, "0x");
    toHex(tsDebug[2]>>32, 8, '0', tmp+2);
    tmp[10] = '.';
    toHex(tsDebug[2], 8, '0', tmp+11);
    tmp[19] = 0;
    end = append(end, "dbg tai:   ");
    end = append(end, tmp);
    end = append(end, "\n\n");

    // tai - utc
    strcpy(tmp, "0x");
    toHex(clkTaiUtcOffset >> 32, 8, '0', tmp + 2);
    tmp[10] = '.';
    toHex(clkTaiUtcOffset, 8, '0', tmp + 11);
    tmp[19] = 0;
    end = append(end, "tai - utc: ");
    end = append(end, tmp);
    end = append(end, "\n");

    // mono - eth
    strcpy(tmp, "0x");
    toHex(clkMonoEth, 8, '0', tmp + 2);
    tmp[10] = 0;
    end = append(end, "mon - eth: ");
    end = append(end, tmp);
    end = append(end, "\n");

    // return size
    return end - body;
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

    // mac router
    macToStr(macRouter, tmp);
    end = append(end, "mac router: ");
    end = append(end, tmp);
    end = append(end, "\n");

    // ip address
    end = append(end, "ip address: ");
    end = addrToStr(ipAddress, end);
    end = append(end, "\n");

    // ip subnet
    end = append(end, "ip subnet: ");
    end = addrToStr(ipSubnet, end);
    end = append(end, "\n");

    // ip subnet
    end = append(end, "ip broadcast: ");
    end = addrToStr(ipBroadcast, end);
    end = append(end, "\n");

    // ip address
    end = append(end, "ip router: ");
    end = addrToStr(ipRouter, end);
    end = append(end, "\n");

    // ip address
    end = append(end, "ip dns: ");
    end = addrToStr(ipDNS, end);
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

    // lease expiration
    tmp[toBase(DHCP_expires(), 10, tmp)] = 0;
    end = append(end, "lease expires: ");
    end = append(end, tmp);
    end = append(end, "\n");

    // return size
    return end - body;
}

unsigned statusGPS(char *body) {
    char tmp[32];
    char *end = body;

    // gps fix
    end = append(end, "fix good: ");
    end = append(end, GPS_hasFix() ? "yes" : "no");
    end = append(end, "\n");

    // latitude
    tmp[fmtFloat(GPS_locLat(), 0, 5, tmp)] = 0;
    end = append(end, "latitude: ");
    end = append(end, tmp);
    end = append(end, " deg\n");

    // longitude
    tmp[fmtFloat(GPS_locLon(), 0, 5, tmp)] = 0;
    end = append(end, "longitude: ");
    end = append(end, tmp);
    end = append(end, " deg\n");

    // altitude
    tmp[fmtFloat(GPS_locAlt(), 0, 2, tmp)] = 0;
    end = append(end, "altitude: ");
    end = append(end, tmp);
    end = append(end, " m\n");

    // clock bias
    tmp[fmtFloat((float) GPS_clkBias(), 0, 0, tmp)] = 0;
    end = append(end, "clock bias: ");
    end = append(end, tmp);
    end = append(end, " ns\n");

    // clock drift
    tmp[fmtFloat((float) GPS_clkDrift(), 0, 0, tmp)] = 0;
    end = append(end, "clock drift: ");
    end = append(end, tmp);
    end = append(end, " ns/s\n");

    // time accuracy
    tmp[toBase(GPS_accTime(), 10, tmp)] = 0;
    end = append(end, "time accuracy: ");
    end = append(end, tmp);
    end = append(end, " ns\n");

    // frequency accuracy
    tmp[toBase(GPS_accFreq(), 10, tmp)] = 0;
    end = append(end, "frequency accuracy: ");
    end = append(end, tmp);
    end = append(end, " ps/s\n");

    return end - body;
}

unsigned statusSystem(char *body) {
    char tmp[32];
    char *end = body;

    // firmware version
    toHex(VERSION_FW, 8, '0', tmp);
    tmp[8] = 0;
    end = append(end, "firmware: 0x");
    end = append(end, tmp);
    end = append(end, "\n");

    // mcu device id
    strcpy(tmp, " 0x");
    toHex(DID0.raw, 8, '0', tmp+3);
    tmp[11] = 0;
    end = append(end, "device id:");
    end = append(end, tmp);
    toHex(DID1.raw, 8, '0', tmp+3);
    tmp[11] = 0;
    end = append(end, tmp);
    end = append(end, "\n");

    // mcu unique id
    end = append(end, "unique id:");
    for(int i = 0; i < 4; i++) {
        toHex(UNIQUEID.WORD[i], 8, '0', tmp+3);
        tmp[11] = 0;
        end = append(end, tmp);
    }
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

    // eeprom status
    EEPROM_seek(0);
    uint32_t eeprom_format = EEPROM_read();
    uint32_t eeprom_version = EEPROM_read();

    // eeprom format
    toHex(eeprom_format, 8, '0', tmp);
    tmp[8] = 0;
    end = append(end, "eeprom format: 0x");
    end = append(end, tmp);
    end = append(end, "\n");

    // eeprom version
    toHex(eeprom_version, 8, '0', tmp);
    tmp[8] = 0;
    end = append(end, "eeprom version: 0x");
    end = append(end, tmp);
    end = append(end, "\n");

    // return size
    return end - body;
}

unsigned statusEEPROM(int block, char *body) {
    char tmp[32];
    char *end = body;

    strcpy(tmp, "block 0x");
    toHex(block, 2, '0', tmp+8);
    tmp[10] = '\n';
    tmp[11] = 0;
    end = append(end, tmp);

    EEPROM_seek(block << 4);
    for(int i = 0; i < 16; i++) {
        uint32_t word = EEPROM_read();
        end = append(end, "  ");
        end = toHexBytes(end, (uint8_t *) &word, sizeof(word));
        end = append(end, "\n");
    }

    // return size
    return end - body;
}
