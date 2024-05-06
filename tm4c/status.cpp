//
// Created by robert on 12/2/22.
//

#include "status.hpp"

#include "gitversion.h"
#include "hw/eeprom.h"
#include "hw/emac.h"
#include "hw/sys.h"
#include "lib/format.hpp"
#include "lib/gps.hpp"
#include "lib/led.hpp"
#include "lib/net.hpp"
#include "lib/run.hpp"
#include "lib/clock/capture.hpp"
#include "lib/clock/comp.hpp"
#include "lib/clock/mono.hpp"
#include "lib/clock/tai.hpp"
#include "lib/net/arp.hpp"
#include "lib/net/dhcp.hpp"
#include "lib/net/ip.hpp"
#include "lib/net/udp.hpp"
#include "lib/net/util.hpp"
#include "lib/ntp/pll.hpp"

#include <cstring>

#define STATUS_PORT (23) // telnet port

namespace status {
    struct [[gnu::packed]] FrameStatus : FrameUdp4 {
        static constexpr int DATA_OFFSET = FrameUdp4::DATA_OFFSET;

        char data[0];

        static auto& from(void *frame) {
            return *static_cast<FrameStatus*>(frame);
        }

        static auto& from(const void *frame) {
            return *static_cast<const FrameStatus*>(frame);
        }
    };

    static_assert(sizeof(FrameStatus) == 42, "FrameStatus must be 42 bytes");

    static void process(uint8_t *frame, int flen);
}

static unsigned statusClock(char *body);
static unsigned statusEEPROM(int block, char *body);
static unsigned statusETH(char *body);
static unsigned statusGPS(char *body);
static unsigned statusSystem(char *body);

unsigned statusSom(char *buffer);

bool hasTerminus(const char *str, int offset) {
    if (str[offset] == 0)
        return true;
    if (str[offset] == '\n')
        return true;
    if (str[offset] == '\r')
        return true;
    return false;
}

void status::init() {
    UDP_register(STATUS_PORT, process);
}

static void status::process(uint8_t *frame, int flen) {
    // restrict length
    if (flen > 128)
        return;
    // map headers
    const auto &request = FrameStatus::from(frame);
    // verify destination
    if (isMyMAC(request.eth.macDst))
        return;
    if (request.ip4.dst != ipAddress)
        return;
    // restrict length
    unsigned size = htons(request.udp.length) - sizeof(HeaderUdp4);
    if (size > 31)
        return;
    // status activity
    LED_act1();

    // get TX buffer
    const int txDesc = NET_getTxDesc();
    uint8_t *txFrame = NET_getTxBuff(txDesc);
    memcpy(txFrame, frame, flen);
    auto &response = FrameStatus::from(txFrame);
    response.returnToSender();

    // get packet body
    char *body = response.data;
    // force null termination
    body[size] = 0;
    if (strncmp(body, "clock", 5) == 0 && hasTerminus(body, 8)) {
        size = statusClock(body);
    }
    else if (strncmp(body, "eeprom", 6) == 0 && hasTerminus(body, 8)) {
        size = statusEEPROM(static_cast<int>(fromHex(body + 6, 2)), body);
    }
    else if (strncmp(body, "ethernet", 8) == 0 && hasTerminus(body, 8)) {
        size = statusETH(body);
    }
    else if (strncmp(body, "gps", 3) == 0 && hasTerminus(body, 3)) {
        size = statusGPS(body);
    }
    else if (strncmp(body, "pll", 3) == 0 && hasTerminus(body, 3)) {
        size = PLL_status(body);
    }
    else if (strncmp(body, "system", 6) == 0 && hasTerminus(body, 6)) {
        size = statusSystem(body);
    }
    else if (strncmp(body, "run", 3) == 0 && hasTerminus(body, 3)) {
        size = runStatus(body);
    }
    else if (strncmp(body, "som", 3) == 0 && hasTerminus(body, 3)) {
        size = statusSom(body);
    }
    else {
        char tmp[32];
        strncpy(tmp, body, size);
        char *end = body;
        end = append(end, "invalid command: ");
        end = append(end, tmp);
        end = append(end, "\n");
        size = end - body;
    }

    // finalize response
    flen = FrameUdp4::DATA_OFFSET + static_cast<int>(size);
    UDP_finalize(txFrame, flen);
    IPv4_finalize(txFrame, flen);
    // transmit response
    NET_transmit(txDesc, flen);
}

static unsigned statusClock(char *body) {
    char tmp[32];
    char *end = body;

    // current time
    const uint64_t mono = clock::monotonic::now();
    strcpy(tmp, "0x");
    toHex(mono >> 32, 8, '0', tmp + 2);
    tmp[10] = '.';
    toHex(mono, 8, '0', tmp + 11);
    tmp[19] = 0;
    end = append(end, "clk mono:  ");
    end = append(end, tmp);
    end = append(end, "\n");

    // current time
    const uint64_t comp = clock::compensated::now();
    strcpy(tmp, "0x");
    toHex(comp >> 32, 8, '0', tmp + 2);
    tmp[10] = '.';
    toHex(comp, 8, '0', tmp + 11);
    tmp[19] = 0;
    end = append(end, "clk comp:  ");
    end = append(end, tmp);
    end = append(end, "\n");

    // TAI time
    const uint64_t tai = clock::tai::now();
    strcpy(tmp, "0x");
    toHex(tai >> 32, 8, '0', tmp + 2);
    tmp[10] = '.';
    toHex(tai, 8, '0', tmp + 11);
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
    clock::capture::ppsGps(pps);
    strcpy(tmp, "0x");
    toHex(pps[0] >> 32, 8, '0', tmp + 2);
    tmp[10] = '.';
    toHex(pps[0], 8, '0', tmp + 11);
    tmp[19] = 0;
    end = append(end, "pps mono:  ");
    end = append(end, tmp);
    end = append(end, "\n");

    strcpy(tmp, "0x");
    toHex(pps[1] >> 32, 8, '0', tmp + 2);
    tmp[10] = '.';
    toHex(pps[1], 8, '0', tmp + 11);
    tmp[19] = 0;
    end = append(end, "pps comp:  ");
    end = append(end, tmp);
    end = append(end, "\n");

    strcpy(tmp, "0x");
    toHex(pps[2] >> 32, 8, '0', tmp + 2);
    tmp[10] = '.';
    toHex(pps[2], 8, '0', tmp + 11);
    tmp[19] = 0;
    end = append(end, "pps tai:   ");
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
    toHex(clock::capture::ppsEthernetRaw(), 8, '0', tmp + 2);
    tmp[10] = 0;
    end = append(end, "mon - eth: ");
    end = append(end, tmp);
    end = append(end, "\n");

    // return size
    return end - body;
}

static unsigned statusETH(char *body) {
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

    // rx crc errors
    tmp[toBase(EMAC0.RXCNTCRCERR, 10, tmp)] = 0;
    end = append(end, "rx crc errors: ");
    end = append(end, tmp);
    end = append(end, "\n");

    // rx overflow errors
    tmp[toBase(NET_getOverflowRx(), 10, tmp)] = 0;
    end = append(end, "rx overflows: ");
    end = append(end, tmp);
    end = append(end, "\n");

    // tx packets
    tmp[toBase(EMAC0.TXCNTGB, 10, tmp)] = 0;
    end = append(end, "tx packets: ");
    end = append(end, tmp);
    end = append(end, "\n");

    // link status
    const int phyStatus = NET_getPhyStatus();
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

static unsigned statusGPS(char *body) {
    char tmp[32];
    char *end = body;

    // gps fix
    end = append(end, "fix good: ");
    end = append(end, gps::hasFix() ? "yes" : "no");
    end = append(end, "\n");

    // latitude
    tmp[fmtFloat(gps::locLat(), 0, 5, tmp)] = 0;
    end = append(end, "latitude: ");
    end = append(end, tmp);
    end = append(end, " deg\n");

    // longitude
    tmp[fmtFloat(gps::locLon(), 0, 5, tmp)] = 0;
    end = append(end, "longitude: ");
    end = append(end, tmp);
    end = append(end, " deg\n");

    // altitude
    tmp[fmtFloat(gps::locAlt(), 0, 2, tmp)] = 0;
    end = append(end, "altitude: ");
    end = append(end, tmp);
    end = append(end, " m\n");

    // clock bias
    tmp[fmtFloat(static_cast<float>(gps::clkBias()), 0, 0, tmp)] = 0;
    end = append(end, "clock bias: ");
    end = append(end, tmp);
    end = append(end, " ns\n");

    // clock drift
    tmp[fmtFloat(static_cast<float>(gps::clkDrift()), 0, 0, tmp)] = 0;
    end = append(end, "clock drift: ");
    end = append(end, tmp);
    end = append(end, " ns/s\n");

    // time accuracy
    tmp[toBase(gps::accTime(), 10, tmp)] = 0;
    end = append(end, "time accuracy: ");
    end = append(end, tmp);
    end = append(end, " ns\n");

    // frequency accuracy
    tmp[toBase(gps::accFreq(), 10, tmp)] = 0;
    end = append(end, "frequency accuracy: ");
    end = append(end, tmp);
    end = append(end, " ps/s\n");

    return end - body;
}

static unsigned statusSystem(char *body) {
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
    toHex(DID0.raw, 8, '0', tmp + 3);
    tmp[11] = 0;
    end = append(end, "device id:");
    end = append(end, tmp);
    toHex(DID1.raw, 8, '0', tmp + 3);
    tmp[11] = 0;
    end = append(end, tmp);
    end = append(end, "\n");

    // mcu unique id
    end = append(end, "unique id:");
    for (int i = 0; i < 4; i++) {
        toHex(UNIQUEID.WORD[i], 8, '0', tmp + 3);
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
    const uint32_t eeprom_format = EEPROM_read();
    const uint32_t eeprom_version = EEPROM_read();

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

    // RAM size
    tmp[toBase(RAM_used(), 10, tmp)] = 0;
    end = append(end, "ram used: ");
    end = append(end, tmp);
    end = append(end, "\n");

    // RAM size
    tmp[toBase(FLASH_used(), 10, tmp)] = 0;
    end = append(end, "flash used: ");
    end = append(end, tmp);
    end = append(end, "\n");

    // return size
    return end - body;
}

static unsigned statusEEPROM(int block, char *body) {
    char tmp[32];
    char *end = body;

    strcpy(tmp, "block 0x");
    toHex(block, 2, '0', tmp + 8);
    tmp[10] = '\n';
    tmp[11] = 0;
    end = append(end, tmp);

    EEPROM_seek(block << 4);
    for (int i = 0; i < 16; i++) {
        uint32_t word = EEPROM_read();
        end = append(end, "  ");
        end = toHexBytes(end, reinterpret_cast<uint8_t*>(&word), sizeof(word));
        end = append(end, "\n");
    }

    // return size
    return end - body;
}
