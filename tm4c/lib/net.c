//
// Created by robert on 4/26/22.
//

#include "../hw/crc.h"
#include "../hw/emac.h"
#include "../hw/interrupts.h"
#include "../hw/gpio.h"
#include "../hw/sys.h"
#include "clk/comp.h"
#include "clk/mono.h"
#include "clk/tai.h"
#include "clk/util.h"
#include "delay.h"
#include "net.h"
#include "net/arp.h"
#include "net/dhcp.h"
#include "net/eth.h"
#include "net/ip.h"
#include "net/util.h"
#include "net/dns.h"
#include "schedule.h"

#define RX_RING_MASK (31)
#define RX_RING_SIZE (32)
#define RX_BUFF_SIZE (1520)

#define TX_RING_MASK (31)
#define TX_RING_SIZE (32)
#define TX_BUFF_SIZE (1520)

static int ptrRX = 0;
static int ptrTX = 0;
static int endTX = 0;
static int phyStatus = 0;

static struct EMAC_RX_DESC rxDesc[RX_RING_SIZE];
static uint8_t rxBuffer[RX_RING_SIZE][RX_BUFF_SIZE];

static uint32_t txCallbackCnt;
static struct { CallbackNetTX call; void *ref; } txCallback[TX_RING_SIZE];
static struct EMAC_TX_DESC txDesc[TX_RING_SIZE];
static uint8_t txBuffer[TX_RING_SIZE][TX_BUFF_SIZE];

static void initDescriptors() {
    // init receive descriptors
    for(int i = 0; i < RX_RING_SIZE; i++) {
        rxDesc[i].BUFF1 = (uint32_t) rxBuffer[i];
        rxDesc[i].BUFF2 = 0;
        rxDesc[i].RDES1.RBS1 = RX_BUFF_SIZE;
        rxDesc[i].RDES1.RBS2 = 0;
        rxDesc[i].RDES1.RER = 0;
        rxDesc[i].RDES0.OWN = 1;
    }
    rxDesc[RX_RING_SIZE-1].RDES1.RER = 1;

    // init transmit descriptors
    for(int i = 0; i < TX_RING_SIZE; i++) {
        txDesc[i].BUFF1 = (uint32_t) txBuffer[i];
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
        txCallback[i].call = 0;
    }
    txDesc[TX_RING_SIZE-1].TDES0.TER = 1;
}

static void initPHY() {
    // configure LEDs
    RCGCGPIO.EN_PORTF = 1;
    delay_cycles_4();
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
    delay_cycles_4();
    while(!PREPHY.RDY0);
    // enable power
    PCEPHY.EN0 = 1;
    delay_cycles_4();
    while(!PREPHY.RDY0);

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
    delay_cycles_4();
    while(!PRCCM.RDY);
    // compute MAC address
    CRC.CTRL.TYPE = CRC_TYPE_04C11DB7;
    CRC.CTRL.INIT = CRC_INIT_ZERO;
    CRC.DIN = UNIQUEID.WORD[0];
    CRC.DIN = UNIQUEID.WORD[1];
    CRC.DIN = UNIQUEID.WORD[2];
    CRC.DIN = UNIQUEID.WORD[3];
    // set MAC address
    uint8_t macAddr[6] = {
            // "TUX" prefix borrowed from tuxgraphics.org
            0x54, 0x55, 0x58,
            (CRC.SEED >> 16) & 0xFF,
            (CRC.SEED >> 8) & 0xFF,
            (CRC.SEED >> 0) & 0xFF
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
    delay_cycles_4();
    while(!PREMAC.RDY0);
    // initialize PHY
    initPHY();
    // wait for DMA reset to complete
    while(EMAC0.DMABUSMOD.SWR);

    initHwAddr();

    // configure DMA
    EMAC0.DMABUSMOD.ATDS = 1;
    EMAC0.RXDLADDR = (uint32_t) rxDesc;
    EMAC0.TXDLADDR = (uint32_t) txDesc;
    EMAC0.DMAOPMODE.ST = 1;
    EMAC0.DMAOPMODE.SR = 1;

    // set frame filter moe
    EMAC0.FRAMEFLTR.PM = 1;
    EMAC0.FRAMEFLTR.VTFE = 1;
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

void ISR_EthernetMAC(void) {
    // process link status changes
    if(EMAC0.PHY.MIS.INT) {
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
        uint16_t temp = EMAC_MII_Read(&EMAC0, MII_ADDR_EPHYBMSR);
        if(temp & 4) phyStatus |= 1;
        return;
    }
}

static void runRx(void *ref) {
    // check for completed receptions
    for(;;) {
        struct EMAC_RX_DESC *pRxDesc = rxDesc + ptrRX;
        // test for ownership
        if (pRxDesc->RDES0.OWN) break;
        // process frame if there was no error
        if (!pRxDesc->RDES0.ES) {
            uint8_t *buffer = (uint8_t *) pRxDesc->BUFF1;
            if (((struct FRAME_ETH *) buffer)->ethType == ETHTYPE_ARP)
                ARP_process(buffer, pRxDesc->RDES0.FL);
            else if (((struct FRAME_ETH *) buffer)->ethType == ETHTYPE_IPv4)
                IPv4_process(buffer, pRxDesc->RDES0.FL);
        }
        // restore ownership to DMA
        pRxDesc->RDES0.OWN = 1;
        // advance pointer
        ptrRX = (ptrRX + 1) & RX_RING_MASK;
    }
}

static void runTxCallback(void *ref) {
    // check for completed transmissions
    while ((endTX != ptrTX) && !txDesc[endTX].TDES0.OWN) {
        // invoke callback
        CallbackNetTX pCall = txCallback[endTX].call;
        if(pCall) {
            (*pCall) (txCallback[endTX].ref, txBuffer[endTX], txDesc[endTX].TDES1.TBS1);
            // clear callback
            txCallback[endTX].call = 0;
            --txCallbackCnt;
        }
        // advance pointer
        endTX = (endTX + 1) & TX_RING_MASK;
    }

    // shutdown thread if there is no more work
    if(txCallbackCnt == 0)
        runRemove(runTxCallback, 0);
}

extern volatile uint16_t ipID;
void NET_init() {
    initDescriptors();
    initMAC();

    // unique IP identifier seed
    ipID = EMAC0.ADDR0.HI.ADDR;
    // initialize network services
    ARP_init();
    DHCP_init();
    DNS_init();

    // schedule RX processing
    runInterval(1u << (32 - 16), runRx, 0);
}

void NET_getMacAddress(char *strAddr) {
    uint8_t mac[6];
    getMAC(mac);
    macToStr(mac, strAddr);
}

int NET_getPhyStatus() {
    return phyStatus;
}

int NET_getTxDesc() {
    if(txDesc[ptrTX].TDES0.OWN)
        return -1;

    int temp = ptrTX;
    ++ptrTX;
    ptrTX &= TX_RING_MASK;
    return temp;
}

uint8_t * NET_getTxBuff(int desc) {
    return (uint8_t *) txDesc[desc & TX_RING_MASK].BUFF1;
}

void NET_setTxCallback(int desc, CallbackNetTX callback, volatile void *ref) {
    txCallback[desc & TX_RING_MASK].call = callback;
    txCallback[desc & TX_RING_MASK].ref = (void *) ref;
}

void NET_transmit(int desc, int len) {
    // restrict transmission length
    if(len < 60) len = 60;
    if(len > TX_BUFF_SIZE) len = TX_BUFF_SIZE;
    // set transmission size
    txDesc[desc & TX_RING_MASK].TDES1.TBS1 = len;
    // release descriptor
    txDesc[desc & TX_RING_MASK].TDES0.OWN = 1;
    // wake TX DMA
    EMAC0.TXPOLLD = 1;
    // schedule callback
    if(txCallback[desc & TX_RING_MASK].call) {
        if(++txCallbackCnt == 1)
            runInterval(1u << (32 - 16), runTxCallback, 0);
    }
}

/**
 * Assemble 64-bit fixed-point timestamps from raw counter value
 * @param timer raw counter value (125 MHz)
 * @param stamps array of 3 64-bit timestamps [ monotonic, compensated, tai ]
 */
static inline void toStamps(uint32_t timer, volatile uint64_t *stamps) {
    // snapshot clock state
    __disable_irq();
    uint32_t monoEth = clkMonoEth;
    uint32_t offset = clkMonoOff;
    uint32_t integer = clkMonoInt;
    __enable_irq();
    // adjust timer offset
    timer += monoEth;
    // assemble timestamps
    stamps[0] = fromClkMono(timer, offset, integer);
    stamps[1] = stamps[0] + corrValue(clkCompRate, (int64_t) (stamps[0] - clkCompRef)) + clkCompOffset;
    stamps[2] = stamps[1] + corrValue(clkTaiRate, (int64_t) (stamps[1] - clkTaiRef)) + clkTaiOffset;
}

void NET_getRxTime(const uint8_t *rxFrame, volatile uint64_t *stamps) {
    // compute descriptor offset
    uint32_t rxId = (rxFrame - rxBuffer[0]) / RX_BUFF_SIZE;
    if(rxId >= RX_RING_SIZE) return;
    // retrieve timestamp
    uint32_t timer;
    timer  = rxDesc[rxId].RTSH * CLK_FREQ;
    timer += rxDesc[rxId].RTSL >> 3;
    // assemble timestamps
    toStamps(timer, stamps);
}

void NET_getTxTime(const uint8_t *txFrame, volatile uint64_t *stamps) {
    // compute descriptor offset
    uint32_t txId = (txFrame - txBuffer[0]) / TX_BUFF_SIZE;
    if(txId >= TX_RING_SIZE) return;
    // retrieve timestamp
    uint32_t timer;
    timer  = txDesc[txId].TTSH * CLK_FREQ;
    timer += txDesc[txId].TTSL >> 3;
    // assemble timestamps
    toStamps(timer, stamps);
}
