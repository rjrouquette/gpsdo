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

#include <algorithm>
#include <memory.h>

// assign ethernet MAC peripheral
static auto &EMAC = EMAC0;
// assign ethernet PHY LED GPIO port
static auto &PHY_GPIO = PORTF;

static constexpr int MTU = 1518;
// 128 bytes = smallest ethernet frame (64-bytes) plus interframe gap (64-bytes)
static constexpr int SEGMENT_SIZE = 128;

// 128 frames = 1.31 ms
static constexpr int RX_RING_SIZE = 128;
// circular buffer modulo mask
static constexpr int RX_RING_MASK = RX_RING_SIZE - 1;

// 128 frames = 1.31 ms
static constexpr int TX_RING_SIZE = 128;
// circular buffer modulo mask
static constexpr int TX_RING_MASK = TX_RING_SIZE - 1;

inline void incrRx(int &ptr) {
    ptr = (ptr + 1) & RX_RING_MASK;
}

inline void incrTx(int &ptr) {
    ptr = (ptr + 1) & TX_RING_MASK;
}

static volatile int rxTail = 0;
static volatile int txHead = 0;
static volatile int txTail = 0;
static volatile int phyStatus = 0;
static volatile uint32_t overflowRx = 0;
static volatile uint32_t overflowTx = 0;

static volatile struct {
    uint32_t lo;
    uint32_t hi;
} rxTime, txTime;

static void *volatile taskRx;
static EMAC_RX_DESC rxDesc[RX_RING_SIZE];
static uint8_t rxBuffer[RX_RING_SIZE][SEGMENT_SIZE];

static void *volatile taskTx;
static EMAC_TX_DESC txDesc[TX_RING_SIZE];
static uint8_t txBuffer[TX_RING_SIZE][SEGMENT_SIZE];

static struct {
    network::CallbackTx call;
    void *ref;
} txCallback[TX_RING_SIZE];


void PTP_process(uint8_t *frame, int size);

static constexpr struct {
    uint16_t ethType;
    void (*handler)(uint8_t *frame, int size);
} registry[] = {
    {ETHTYPE_ARP, ARP_process},
    {ETHTYPE_PTP, PTP_process},
    {ETHTYPE_IP4, IPv4_process},
};

static void initRx() {
    // init receive descriptors
    for (int i = 0; i < RX_RING_SIZE; i++) {
        rxDesc[i].BUFF1 = reinterpret_cast<uint32_t>(rxBuffer[i]);
        rxDesc[i].BUFF2 = 0;
        rxDesc[i].RDES1.RBS1 = SEGMENT_SIZE;
        rxDesc[i].RDES1.RBS2 = 0;
        rxDesc[i].RDES1.RER = 0;
        rxDesc[i].RDES1.RCH = 0;
        rxDesc[i].RDES0.OWN = 1;
    }
    rxDesc[RX_RING_MASK].RDES1.RER = 1;
}

static void restartRx() {
    // reset receive descriptors
    initRx();
    // reset receive pointer
    rxTail = 0;
    // restart DMA controller
    EMAC.RXDLADDR = reinterpret_cast<uint32_t>(rxDesc);
    EMAC.DMAOPMODE.SR = 1;
}

static void initDescriptors() {
    initRx();

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
    PHY_GPIO.LOCK = GPIO_LOCK_KEY;
    PHY_GPIO.CR = 0x11u;
    // configure pins
    PHY_GPIO.DIR = 0x11u;
    PHY_GPIO.DR8R = 0x11u;
    PHY_GPIO.PCTL.PMC0 = 0x5;
    PHY_GPIO.PCTL.PMC4 = 0x5;
    PHY_GPIO.AFSEL.ALT0 = 1;
    PHY_GPIO.AFSEL.ALT4 = 1;
    PHY_GPIO.DEN = 0x11u;
    // lock GPIO config
    PHY_GPIO.CR = 0;
    PHY_GPIO.LOCK = 0;

    // prevent errant transmissions
    EMAC.PC.PHYHOLD = 1;
    // enable clock
    RCGCEPHY.EN0 = 1;
    delay::cycles(4);
    while (!PREPHY.RDY0) {}
    // enable power
    PCEPHY.EN0 = 1;
    delay::cycles(4);
    while (!PREPHY.RDY0) {}

    // enable PHY interrupt for relevant link changes
    uint16_t temp = EMAC_MII_Read(&EMAC, MII_ADDR_EPHYMISR1);
    temp |= 0x3Cu;
    EMAC_MII_Write(&EMAC, MII_ADDR_EPHYMISR1, temp);
    // enable PHY interrupt
    EMAC.PHY.IM.INT = 1;

    // complete configuration
    EMAC.PC.PHYHOLD = 0;
    temp = EMAC_MII_Read(&EMAC, MII_ADDR_EPHYCFG1);
    temp |= 1u << 15u;
    EMAC_MII_Write(&EMAC, MII_ADDR_EPHYCFG1, temp);
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
    EMAC_setMac(&(EMAC.ADDR0), macAddr);
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
    while (EMAC.DMABUSMOD.SWR) {}

    initHwAddr();

    // configure DMA
    EMAC.DMABUSMOD.ATDS = 1;
    EMAC.RXDLADDR = reinterpret_cast<uint32_t>(rxDesc);
    EMAC.TXDLADDR = reinterpret_cast<uint32_t>(txDesc);
    EMAC.DMAOPMODE.ST = 1;
    EMAC.DMAOPMODE.SR = 1;
    // enable RX/TX interrupts
    EMAC.DMAIM.OVE = 1;
    EMAC.DMAIM.RIE = 1;
    EMAC.DMAIM.TIE = 1;
    EMAC.DMAIM.NIE = 1;
    EMAC.DMAIM.AIE = 1;

    // set frame filter mode
    EMAC.FRAMEFLTR.RA = 1;
    // verify all checksum
    EMAC.CFG.IPC = 1;
    // prevent loopback of data in half-duplex mode
    EMAC.CFG.DRO = 1;
    // strip checksum from received frames
    EMAC.CFG.CST = 1;
    // replace source MAC address in transmissions
    EMAC.CFG.SADDR = EMAC_SADDR_REP0;

    // start transmitter
    EMAC.CFG.TE = 1;
    // start receiver
    EMAC.CFG.RE = 1;

    // re-enable flash prefetch per errata
    FLASHCONF.FPFOFF = 0;
}

void ISR_EthernetMAC() {
    // process link status changes
    if (EMAC.PHY.MIS.INT) {
        // clear interrupt
        EMAC_MII_Read(&EMAC, MII_ADDR_EPHYMISR1);
        EMAC.PHY.MIS.INT = 1;
        // fetch status
        phyStatus = EMAC_MII_Read(&EMAC, MII_ADDR_EPHYSTS);
        phyStatus ^= 0x0002;
        // set speed
        EMAC.CFG.FES = (phyStatus >> 1u) & 1u;
        // set duplex
        EMAC.CFG.DUPM = (phyStatus >> 2u) & 1u;
        // link status bit in EPHYSTS is buggy, but EPHYBMSR works
        const uint16_t temp = EMAC_MII_Read(&EMAC, MII_ADDR_EPHYBMSR);
        if (temp & 4)
            phyStatus |= 1;
        return;
    }

    // copy and clear interrupt flags
    const auto DMARIS = const_cast<EMAC_MAP&>(EMAC).DMARIS;
    EMAC.DMARIS.raw = DMARIS.raw;

    // check for receive interrupt
    if (DMARIS.RI)
        runWake(taskRx);

    // check for transmit interrupt
    if (DMARIS.TI)
        runWake(taskTx);

    // check for overflow interrupt
    if (DMARIS.OVF) {
        ++overflowRx;
        EMAC.DMAOPMODE.SR = 0;
        runWake(taskRx);
    }
}

static bool processFrameRx() {
    // get ring state
    const int tail = rxTail;

    // determine if a complete frame is available
    auto ptr = tail;
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
        ptr = tail;
        while (ptr != end) {
            // restore DMA ownership
            rxDesc[ptr].RDES0.OWN = 1;
            // advance ring pointer
            incrRx(ptr);
        }
        rxTail = ptr;
        return true;
    }

    // assemble full frame
    uint8_t buffer[last.RDES0.FL];
    rxTime.lo = last.RTSL;
    rxTime.hi = last.RTSH;
    int length = 0;
    ptr = tail;
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
    rxTail = ptr;

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
    while (processFrameRx()) {}

    // check for receiver reset
    if (!EMAC.DMAOPMODE.SR)
        restartRx();
}

static bool processFrameTx() {
    // get ring state
    const int head = txHead;
    const int tail = txTail;

    // check for completed transmission
    int ptr = tail;
    unsigned size = 0;
    for (;;) {
        const auto status = txDesc[ptr].TDES0;
        // check for end of ring or DMA ownership
        if (ptr == head || status.OWN)
            return false;
        // accumulate size
        size += txDesc[ptr].TDES1.TBS1;
        // check for last segment flag
        if (status.LS)
            break;
        incrTx(ptr);
    }

    // reassemble frame
    const auto last = ptr;
    incrTx(ptr);
    const int end = ptr;

    // check for callback
    const auto pCall = txCallback[last].call;
    if (
        pCall == nullptr
    ) {
        // update ring buffer state
        txTail = end;
        return true;
    }

    // clear callback
    txCallback[last].call = nullptr;

    // assemble full frame
    uint8_t buffer[size];
    txTime.lo = txDesc[last].TTSL;
    txTime.hi = txDesc[last].TTSH;
    size = 0;
    ptr = tail;
    while (ptr != end) {
        // copy segment contents
        const auto segment = txDesc[ptr].TDES1.TBS1;
        memcpy(buffer + size, txBuffer[ptr], segment);
        size += segment;
        // advance ring pointer
        incrTx(ptr);
    }

    // update ring buffer state
    txTail = end;

    // invoke callback
    (*pCall)(txCallback[last].ref, buffer, static_cast<int>(size));
    return true;
}

static void runTx(void *ref) {
    while (processFrameTx()) {}
}

extern volatile uint16_t ipID;

void network::init() {
    // initialize ring buffers
    initDescriptors();
    // create RX/TX threads
    taskRx = runWait(runRx, nullptr);
    taskTx = runWait(runTx, nullptr);
    // initialize MAC and PHY
    initMAC();

    // unique IP identifier seed
    ipID = EMAC.ADDR0.HI.ADDR;
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

uint32_t network::getOverflowRx() {
    return overflowRx;
}

uint32_t network::getOverflowTx() {
    return overflowTx;
}

bool network::transmit(const uint8_t *frame, int size, const CallbackTx callback, void *ref) {
    // restrict transmission length
    if (size > MTU)
        return false;

    // get ring state
    const int head = txHead;
    const int tail = (txTail - 1) & RX_RING_MASK;

    // check for overflow
    if (((tail - head) & TX_RING_MASK) < ((size + SEGMENT_SIZE - 1) / SEGMENT_SIZE)) {
        ++overflowTx;
        return false;
    }

    int ptr = head;
    for (;;) {
        const int segment = std::min(size, SEGMENT_SIZE);

        // copy data
        memcpy(txBuffer[ptr], frame, segment);
        // set segment size
        txDesc[ptr].TDES1.TBS1 = segment;
        txDesc[ptr].TDES0.FS = 0;
        txDesc[ptr].TDES0.LS = 0;
        txDesc[ptr].TDES0.IC = 0;
        txDesc[ptr].TDES0.TTSE = 0;

        frame += segment;
        size -= segment;
        if (size == 0)
            break;

        // mark as intermediate segment
        // advance segment pointer
        incrTx(ptr);
    }

    // mark first segment
    txDesc[head].TDES0.FS = 1;
    txDesc[head].TDES0.TTSE = 1;

    // mark last segment
    txDesc[ptr].TDES0.LS = 1;
    txDesc[ptr].TDES0.IC = 1;

    // set callback
    txCallback[ptr].call = callback;
    txCallback[ptr].ref = ref;

    // update ring buffer state
    incrTx(ptr);
    txHead = ptr;

    // mark segments for transmission
    while (ptr != head) {
        ptr = (ptr - 1) & TX_RING_MASK;
        txDesc[ptr].TDES0.OWN = 1;
    }

    // wake TX DMA
    EMAC.TXPOLLD = 1;
    return true;
}

/**
 * Reduce a split timestamp to the raw counter value of the monotonic clock.
 * @param seconds seconds
 * @param nanos nanoseconds
 * @return monotonic clock raw timer value
 */
inline uint32_t nanosToRaw(const uint32_t seconds, const uint32_t nanos) {
    return seconds * CLK_FREQ + nanos / CLK_NANOS;
}

void network::getRxTime(uint64_t *stamps) {
    // assemble timestamps
    clock::capture::rawToFull(
        clock::capture::ppsEthernetRaw() + nanosToRaw(rxTime.hi, rxTime.lo),
        stamps
    );
}

void network::getTxTime(uint64_t *stamps) {
    // assemble timestamps
    clock::capture::rawToFull(
        clock::capture::ppsEthernetRaw() + nanosToRaw(txTime.hi, txTime.lo),
        stamps
    );
}


// defined as a weak reference so it may be overridden
__attribute__((weak))
void PTP_process(uint8_t *frame, const int size) {
    __asm volatile("nop");
}
