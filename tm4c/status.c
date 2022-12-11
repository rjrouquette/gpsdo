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
#include "ntp.h"
#include "lib/gps.h"

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
    } else if(strncmp(body, "system", 6) == 0 && hasTerminus(body, 6)) {
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
    end = append(end, "ip address: ");
    end = addrToStr(ipAddress, end);
    end = append(end, "\n");

    // ip subnet
    end = append(end, "ip subnet: ");
    end = addrToStr(ipSubnet, end);
    end = append(end, "\n");

    // ip address
    end = append(end, "ip gateway: ");
    end = addrToStr(ipGateway, end);
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
    tmp[toBase(GPS_accTime(), 10, tmp)] = 0;
    end = append(end, "frequency accuracy: ");
    end = append(end, tmp);
    end = append(end, " ps/s\n");

    return end - body;
}

unsigned statusGPSDO(char *body) {
    char tmp[32];
    char *end = body;

    // pll lock
    end = append(end, "pps locked: ");
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

    // PLL trimming
    tmp[fmtFloat(GPSDO_pllTrim() * 1e6f, 0, 4, tmp)] = 0;
    end = append(end, "  - pll trimming: ");
    end = append(end, tmp);
    end = append(end, " ppm\n");

    // temperature compensation
    tmp[fmtFloat(GPSDO_compValue() * 1e6f, 0, 4, tmp)] = 0;
    end = append(end, "compensation: ");
    end = append(end, tmp);
    end = append(end, " ppm\n");

    // temperature
    tmp[fmtFloat(GPSDO_temperature(), 0, 3, tmp)] = 0;
    end = append(end, "  - temperature: ");
    end = append(end, tmp);
    end = append(end, " C\n");

    // temperature offset
    tmp[fmtFloat(GPSDO_compBias(), 0, 3, tmp)] = 0;
    end = append(end, "  - bias: ");
    end = append(end, tmp);
    end = append(end, " C\n");

    // temperature coefficient
    tmp[fmtFloat(GPSDO_compCoeff() * 1e6f, 0, 4, tmp)] = 0;
    end = append(end, "  - coefficient: ");
    end = append(end, tmp);
    end = append(end, " ppm/C\n");

    // temperature offset
    tmp[fmtFloat(GPSDO_compOffset() * 1e6f, 0, 4, tmp)] = 0;
    end = append(end, "  - offset: ");
    end = append(end, tmp);
    end = append(end, " ppm\n");

    // return size
    return end - body;
}

unsigned statusNTP(char *body) {
    char tmp[32];
    char *end = body;

    // TAI time
    uint64_t tai = CLK_GPS();
    strcpy(tmp, "0x");
    toHex(tai>>32, 8, '0', tmp+2);
    tmp[10] = '.';
    toHex(tai, 8, '0', tmp+11);
    tmp[19] = 0;
    end = append(end, "gps epoch:   ");
    end = append(end, tmp);
    end = append(end, "\n");

    // NTP offset
    uint64_t offset = NTP_offset();
    strcpy(tmp, "0x");
    toHex(offset>>32, 8, '0', tmp+2);
    tmp[10] = '.';
    toHex(offset, 8, '0', tmp+11);
    tmp[19] = 0;
    end = append(end, "ntp offset:  ");
    end = append(end, tmp);
    end = append(end, "\n");

    end = NTP_servers(end);

    end = append(end, "clock stratum: ");
    tmp[toBase(NTP_clockStratum(), 10, tmp)] = 0;
    end = append(end, tmp);
    end = append(end, "\n");

    end = append(end, "clock offset: ");
    tmp[fmtFloat(NTP_clockOffset() * 1e3f, 0, 3, tmp)] = 0;
    end = append(end, tmp);
    end = append(end, " ms\n");

    end = append(end, "clock drift: ");
    tmp[fmtFloat(NTP_clockDrift() * 1e6f, 0, 3, tmp)] = 0;
    end = append(end, tmp);
    end = append(end, " ppm\n");

    end = append(end, "leap indicator: ");
    tmp[toBase(NTP_leapIndicator(), 10, tmp)] = 0;
    end = append(end, tmp);
    end = append(end, "\n");


    // return size
    return end - body;
}

#include "gitversion.h"
unsigned statusSystem(char *body) {
    char tmp[32];
    char *end = body;

    // firmware version
    strncpy(tmp, VERSION_GIT, 8);
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

    // return size
    return end - body;
}
