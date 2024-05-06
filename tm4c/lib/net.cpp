//
// Created by robert on 4/26/22.
//

#include "net.hpp"

#include "delay.hpp"
#include "led.hpp"
#include "run.hpp"
#include "../hw/crc.h"
#include "../hw/emac.h"
#include "../hw/gpio.h"
#include "../hw/interrupts.h"
#include "../hw/sys.h"
#include "clock/capture.hpp"
#include "net/arp.hpp"
#include "net/dhcp.hpp"
#include "net/dns.hpp"
#include "net/eth.hpp"
#include "net/ip.hpp"
#include "net/util.hpp"

#include <memory.h>

// 128 frames = 1.31 ms
static constexpr int RX_RING_SIZE = 128;
// circular buffer modulo mask
static constexpr int RX_RING_MASK = RX_RING_SIZE - 1;
// 128 bytes = smallest ethernet frame (64-bytes) plus interframe gap (64-bytes)
static constexpr int RX_BUFF_SIZE = 128;

static constexpr int TX_RING_SIZE = 32;
static constexpr int TX_RING_MASK = TX_RING_SIZE - 1;
static constexpr int TX_BUFF_SIZE = 1520;

inline void incrRx(int &ptr) {
    ptr = (ptr + 1) & RX_RING_MASK;
}

inline void incrTx(int &ptr) {
    ptr = (ptr + 1) & TX_RING_MASK;
}

static volatile int ptrRX = 0;
static volatile int ptrTX = 0;
static volatile int endTX = 0;
static volatile int phyStatus = 0;
static volatile uint32_t overflowRx = 0;

static void *volatile taskRx;
static EMAC_RX_DESC rxDesc[RX_RING_SIZE];
static uint8_t rxBuffer[RX_RING_SIZE][RX_BUFF_SIZE];

static volatile struct {
    uint32_t lo;
    uint32_t hi;
} rxTimestamp;

static struct {
    CallbackNetTX call;
    void *ref;
} txCallback[TX_RING_SIZE];

static void *volatile taskTx;
static EMAC_TX_DESC txDesc[TX_RING_SIZE];
static uint8_t txBuffer[TX_RING_SIZE][TX_BUFF_SIZE];


void PTP_process(uint8_t *frame, int flen);

static constexpr struct {
    uint16_t ethType;
    void (*handler)(uint8_t *frame, int flen);
} registry[] = {
    {ETHTYPE_ARP, ARP_process},
    {ETHTYPE_PTP, PTP_process},
    {ETHTYPE_IP4, IPv4_process},
};

static void initDescriptors() {
    // init receive descriptors
    for (int i = 0; i < RX_RING_SIZE; i++) {
        rxDesc[i].BUFF1 = reinterpret_cast<uint32_t>(rxBuffer[i]);
        rxDesc[i].BUFF2 = 0;
        rxDesc[i].RDES1.RBS1 = RX_BUFF_SIZE;
        rxDesc[i].RDES1.RBS2 = 0;
        rxDesc[i].RDES1.RER = 0;
        rxDesc[i].RDES1.RCH = 0;
        rxDesc[i].RDES0.OWN = 1;
    }
    rxDesc[RX_RING_MASK].RDES1.RER = 1;

    // init transmit descriptors
    for (int i = 0; i < TX_RING_SIZE; i++) {
        txDesc[i].BUFF1 = reinterpret_cast<uint32_t>(txBuffer[i]);
        txDesc[i].BUFF2 = 0;
        txDesc[i].TDES1.TBS1 = 0;
        txDesc[i].TDES1.TBS2 = 0;
        // replace source address
        txDesc[i].TDES1.SAIC = EMAC_SADDR_REP0;
        // not end-of-ring
        txDesc[i].TDES0.TER = 0;
        // mark descriptor as incomplete
        txDesc[i].TDES0.OWN = 0;
        // each descriptor is one frame
        txDesc[i].TDES0.FS = 1;
        txDesc[i].TDES0.LS = 1;
        // capture timestamp
        txDesc[i].TDES0.TTSE = 1;
        // clear callback
        txCallback[i].call = nullptr;
    }
    txDesc[TX_RING_MASK].TDES0.TER = 1;
}

static void initPHY() {
    // configure LEDs
    RCGCGPIO.EN_PORTF = 1;
    delay::cycles(4);
    // unlock GPIO config
    PORTF.LOCK = GPIO_LOCK_KEY;
    PORTF.CR = 0x11u;
    // configure pins
    PORTF.DIR = 0x11u;
    PORTF.DR8R = 0x11u;
    PORTF.PCTL.PMC0 = 0x5;
    PORTF.PCTL.PMC4 = 0x5;
    PORTF.AFSEL.ALT0 = 1;
    PORTF.AFSEL.ALT4 = 1;
    PORTF.DEN = 0x11u;
    // lock GPIO config
    PORTF.CR = 0;
    PORTF.LOCK = 0;

    // prevent errant transmissions
    EMAC0.PC.PHYHOLD = 1;
    // enable clock
    RCGCEPHY.EN0 = 1;
    delay::cycles(4);
    while (!PREPHY.RDY0) {}
    // enable power
    PCEPHY.EN0 = 1;
    delay::cycles(4);
    while (!PREPHY.RDY0) {}

    // enable PHY interrupt for relevant link changes
    uint16_t temp = EMAC_MII_Read(&EMAC0, MII_ADDR_EPHYMISR1);
    temp |= 0x3Cu;
    EMAC_MII_Write(&EMAC0, MII_ADDR_EPHYMISR1, temp);
    // enable PHY interrupt
    EMAC0.PHY.IM.INT = 1;

    // complete configuration
    EMAC0.PC.PHYHOLD = 0;
    temp = EMAC_MII_Read(&EMAC0, MII_ADDR_EPHYCFG1);
    temp |= 1u << 15u;
    EMAC_MII_Write(&EMAC0, MII_ADDR_EPHYCFG1, temp);
}

static void initHwAddr() {
    // enable CRC module
    RCGCCCM.EN = 1;
    delay::cycles(4);
    while (!PRCCM.RDY) {}
    // compute MAC address
    CRC.CTRL.TYPE = CRC_TYPE_04C11DB7;
    CRC.CTRL.INIT = CRC_INIT_ZERO;
    CRC.DIN = UNIQUEID.WORD[0];
    CRC.DIN = UNIQUEID.WORD[1];
    CRC.DIN = UNIQUEID.WORD[2];
    CRC.DIN = UNIQUEID.WORD[3];
    // set MAC address
    const uint8_t macAddr[6] = {
        // "TUX" prefix borrowed from tuxgraphics.org
        0x54, 0x55, 0x58,
        static_cast<uint8_t>(CRC.SEED >> 16),
        static_cast<uint8_t>(CRC.SEED >> 8),
        static_cast<uint8_t>(CRC.SEED >> 0)
    };
    EMAC_setMac(&(EMAC0.ADDR0), macAddr);
    // disable CRC module
    RCGCCCM.EN = 0;
}

static void initMAC() {
    // disable flash prefetch per errata
    FLASHCONF.FPFOFF = 1;
    // enable clock
    RCGCEMAC.EN0 = 1;
    delay::cycles(4);
    while (!PREMAC.RDY0) {}
    // initialize PHY
    initPHY();
    // wait for DMA reset to complete
    while (EMAC0.DMABUSMOD.SWR) {}

    initHwAddr();

    // configure DMA
    EMAC0.DMABUSMOD.ATDS = 1;
    EMAC0.RXDLADDR = reinterpret_cast<uint32_t>(rxDesc);
    EMAC0.TXDLADDR = reinterpret_cast<uint32_t>(txDesc);
    EMAC0.DMAOPMODE.ST = 1;
    EMAC0.DMAOPMODE.SR = 1;
    // enable RX/TX interrupts
    EMAC0.DMAIM.RIE = 1;
    EMAC0.DMAIM.TIE = 1;
    EMAC0.DMAIM.NIE = 1;

    // set frame filter mode
    EMAC0.FRAMEFLTR.RA = 1;
    // verify all checksum
    EMAC0.CFG.IPC = 1;
    // prevent loopback of data in half-duplex mode
    EMAC0.CFG.DRO = 1;
    // strip checksum from received frames
    EMAC0.CFG.CST = 1;
    // replace source MAC address in transmissions
    EMAC0.CFG.SADDR = EMAC_SADDR_REP0;

    // start transmitter
    EMAC0.CFG.TE = 1;
    // start receiver
    EMAC0.CFG.RE = 1;

    // re-enable flash prefetch per errata
    FLASHCONF.FPFOFF = 0;
}

void ISR_EthernetMAC() {
    // process link status changes
    if (EMAC0.PHY.MIS.INT) {
        // clear interrupt
        EMAC_MII_Read(&EMAC0, MII_ADDR_EPHYMISR1);
        EMAC0.PHY.MIS.INT = 1;
        // fetch status
        phyStatus = EMAC_MII_Read(&EMAC0, MII_ADDR_EPHYSTS);
        phyStatus ^= 0x0002;
        // set speed
        EMAC0.CFG.FES = (phyStatus >> 1u) & 1u;
        // set duplex
        EMAC0.CFG.DUPM = (phyStatus >> 2u) & 1u;
        // link status bit in EPHYSTS is buggy, but EPHYBMSR works
        const uint16_t temp = EMAC_MII_Read(&EMAC0, MII_ADDR_EPHYBMSR);
        if (temp & 4)
            phyStatus |= 1;
        return;
    }

    // copy and clear interrupt flags
    const auto DMARIS = const_cast<EMAC_MAP&>(EMAC0).DMARIS;
    EMAC0.DMARIS.raw = DMARIS.raw;

    // check for receive interrupt
    if (DMARIS.RI)
        runWake(taskRx);

    // check for transmit interruot
    if (DMARIS.TI)
        runWake(taskTx);

    // check for overflow interrupt
    if (DMARIS.OVF) {
        ++overflowRx;
    }
}

static bool processFrame() {
    // determine if a complete frame is available
    auto ptr = ptrRX;
    for (;;) {
        const auto status = rxDesc[ptr].RDES0;
        // check for DMA ownership
        if (status.OWN)
            return false;
        // check for last segment flag
        if (status.LS)
            break;
        incrRx(ptr);
    }

    // load last descriptor
    const auto &last = rxDesc[ptr];
    incrRx(ptr);
    const auto end = ptr;

    // discard frames with errors
    if (last.RDES0.ES) {
        ptr = ptrRX;
        while (ptr != end) {
            // restore DMA ownership
            rxDesc[ptr].RDES0.OWN = 1;
            // advance ring pointer
            incrRx(ptr);
        }
        ptrRX = ptr;
        return true;
    }

    // assemble full frame
    uint8_t buffer[last.RDES0.FL];
    rxTimestamp.lo = last.RTSL;
    rxTimestamp.hi = last.RTSH;
    int length = 0;
    ptr = ptrRX;
    while (ptr != end) {
        auto &desc = rxDesc[ptr];
        // copy segment contents
        const auto accLength = static_cast<int>(desc.RDES0.FL);
        memcpy(buffer + length, rxBuffer[ptr], accLength - length);
        length = accLength;
        // restore DMA ownership
        desc.RDES0.OWN = 1;
        // advance ring pointer
        incrRx(ptr);
    }
    ptrRX = ptr;

    // only process the frame if the source MAC is not a broadcast address
    // (prevents packet amplification attacks)
    if (
        const auto headerEth = reinterpret_cast<HeaderEthernet*>(buffer);
        !(headerEth->macSrc[0] & 1)
    ) {
        const auto ethType = headerEth->ethType;
        // dispatch frame processor
        for (const auto &entry : registry) {
            if (ethType == entry.ethType) {
                (*entry.handler)(buffer, length);
                break;
            }
        }
    }
    return true;
}

static void runRx(void *ref) {
    // check for completed receptions
    while (processFrame()) {}
}

static void runTx(void *ref) {
    // check for completed transmissions
    const int ptr = ptrTX;
    int end = endTX;
    while ((end != ptr) && !txDesc[end].TDES0.OWN) {
        // check for callback
        if (
            const auto pCall = txCallback[end].call;
            pCall != nullptr
        ) {
            // invoke callback
            (*pCall)(txCallback[end].ref, txBuffer[end], txDesc[end].TDES1.TBS1);
            // clear callback
            txCallback[end].call = nullptr;
        }
        // advance pointer
        incrTx(end);
    }
    endTX = end;
}

extern volatile uint16_t ipID;

void NET_init() {
    // initialize ring buffers
    initDescriptors();
    // create RX/TX threads
    taskRx = runWait(runRx, nullptr);
    taskTx = runWait(runTx, nullptr);
    // initialize MAC and PHY
    initMAC();

    // unique IP identifier seed
    ipID = EMAC0.ADDR0.HI.ADDR;
    // initialize network services
    ARP_init();
    DHCP_init();
    DNS_init();
}

void NET_getMacAddress(char *strAddr) {
    uint8_t mac[6];
    getMAC(mac);
    macToStr(mac, strAddr);
}

int NET_getPhyStatus() {
    return phyStatus;
}

uint32_t NET_getOverflowRx() {
    return overflowRx;
}

int NET_getTxDesc() {
    const int ptr = ptrTX;
    if (txDesc[ptr].TDES0.OWN)
        faultBlink(4, 1);

    int temp = ptr;
    incrTx(temp);
    ptrTX = temp;
    return ptr;
}

uint8_t* NET_getTxBuff(const int desc) {
    return txBuffer[desc & TX_RING_MASK];
}

void NET_setTxCallback(int desc, CallbackNetTX callback, volatile void *ref) {
    desc &= TX_RING_MASK;
    txCallback[desc].call = callback;
    txCallback[desc].ref = const_cast<void*>(ref);
}

void NET_transmit(int desc, int len) {
    desc &= TX_RING_MASK;
    // restrict transmission length
    if (len < 60)
        len = 60;
    if (len > TX_BUFF_SIZE)
        faultBlink(4, 2);
    // set transmission size
    txDesc[desc].TDES1.TBS1 = len;
    // release descriptor
    txDesc[desc].TDES0.IC = 1;
    txDesc[desc].TDES0.OWN = 1;
    // wake TX DMA
    EMAC0.TXPOLLD = 1;
}

/**
 * Reduce a split timestamp into the raw counter value of the monotonic clock.
 * @param seconds seconds
 * @param nanos nanoseconds
 * @return monotonic clock raw timer value
 */
inline uint32_t nanosToRaw(const uint32_t seconds, const uint32_t nanos) {
    return seconds * CLK_FREQ + nanos / CLK_NANOS;
}

void NET_getRxTime(uint64_t *stamps) {
    // assemble timestamps
    clock::capture::rawToFull(
        clock::capture::ppsEthernetRaw() + nanosToRaw(rxTimestamp.hi, rxTimestamp.lo),
        stamps
    );
}

void NET_getTxTime(const uint8_t *txFrame, uint64_t *stamps) {
    // compute descriptor offset
    const int i = (txFrame - txBuffer[0]) / TX_BUFF_SIZE;
    clock::capture::rawToFull(
        clock::capture::ppsEthernetRaw() + nanosToRaw(txDesc[i].TTSH, txDesc[i].TTSL),
        stamps
    );
}


// defined as a weak reference so it may be overriden
__attribute__((weak))
void PTP_process(uint8_t *frame, int flen) {
    __asm volatile("nop");
}
