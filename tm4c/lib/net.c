//
// Created by robert on 4/26/22.
//

#include "../hw/crc.h"
#include "../hw/emac.h"
#include "../hw/interrupts.h"
#include "../hw/gpio.h"
#include "../hw/sys.h"
#include "../lib/delay.h"
#include "../lib/format.h"
#include "net.h"
#include "net/arp.h"
#include "net/dhcp.h"
#include "net/eth.h"
#include "net/ip.h"
#include "net/udp.h"
#include "net/util.h"

#define RX_RING_SIZE (16)
#define RX_BUFF_SIZE (1524)

#define TX_RING_SIZE (4)
#define TX_BUFF_SIZE (1524)

static volatile uint8_t ptrRX = 0;
static volatile uint8_t ptrTX = 0;
static volatile uint16_t phyStatus = 0;

static volatile struct EMAC_RX_DESC rxDesc[RX_RING_SIZE];
static volatile uint8_t rxBuffer[RX_RING_SIZE][RX_BUFF_SIZE];

static volatile struct EMAC_TX_DESC txDesc[TX_RING_SIZE];
static volatile uint8_t txBuffer[TX_RING_SIZE][TX_BUFF_SIZE];

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
    while(!PREPHY.RDY0);
    // enable power
    PCEPHY.EN0 = 1;
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

static void initPPS() {
    // configure PPS
    RCGCGPIO.EN_PORTG = 1;
    delay_cycles_4();
    // unlock GPIO config
    PORTG.LOCK = GPIO_LOCK_KEY;
    PORTG.CR = 0x01u;
    // configure pins
    PORTG.DIR = 0x01u;
    PORTG.DR8R = 0x01u;
    PORTG.PCTL.PMC0 = 0x5;
    PORTG.AFSEL.ALT0 = 1;
    PORTG.DEN = 0x01u;
    // lock GPIO config
    PORTG.CR = 0;
    PORTG.LOCK = 0;

    // configure PTP
    EMAC0.TIMSTCTRL.TSMAST = 1;
    EMAC0.TIMSTCTRL.PTPIPV4 = 1;
    EMAC0.TIMSTCTRL.PTPIPV6 = 1;
    EMAC0.TIMSTCTRL.PTPVER2 = 1;
    EMAC0.TIMSTCTRL.ALLF = 1;
    EMAC0.TIMSTCTRL.DGTLBIN = 1;
    EMAC0.TIMSTCTRL.TSEN = 1;
    // 25MHz = 40ns
    EMAC0.SUBSECINC.SSINC = 40;

    // PTP clock enabled
    EMAC0.CC.PTPCEN = 1;
}

static void initMAC() {
    // disable flash prefetch per errata
    FLASHCONF.FPFOFF = 1;
    // enable CRC module
    RCGCCCM.EN = 1;
    while(!PRCCM.RDY);
    // enable clock
    RCGCEMAC.EN0 = 1;
    while(!PREMAC.RDY0);
    // initialize PHY
    initPHY();
    // wait for DMA reset to complete
    while(EMAC0.DMABUSMOD.SWR);

    initPPS();

    // compute MAC address
    CRC.CTRL.TYPE = CRC_TYPE_04C11DB7;
    CRC.CTRL.INIT = CRC_INIT_ZERO;
    CRC.DIN = UNIQUEID.WORD[0];
    CRC.DIN = UNIQUEID.WORD[1];
    CRC.DIN = UNIQUEID.WORD[2];
    CRC.DIN = UNIQUEID.WORD[3];
    // set MAC address (byte-order is reversed)
    EMAC0.ADDR0.HI.ADDR = ((CRC.SEED & 0xFF) << 8) | ((CRC.SEED  >> 8) & 0xFF);
    EMAC0.ADDR0.LO = (((CRC.SEED >> 16) & 0xFF) << 24) | 0x585554;

    // configure DMA
    EMAC0.DMABUSMOD.ATDS = 1;
    EMAC0.RXDLADDR = (uint32_t) rxDesc;
    EMAC0.TXDLADDR = (uint32_t) txDesc;
    EMAC0.DMAOPMODE.ST = 1;
    EMAC0.DMAOPMODE.SR = 1;

    EMAC0.FRAMEFLTR.PM = 1;
    EMAC0.FRAMEFLTR.VTFE = 1;
    EMAC0.CFG.IPC = 1;
    EMAC0.CFG.DRO = 1;
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

extern volatile uint16_t ipID;
void NET_init() {
    initDescriptors();
    initMAC();

    // unique IP identifier seed
    ipID = EMAC0.ADDR0.HI.ADDR;
    // register DHCP client port
    UDP_register(DHCP_PORT_CLI, DHCP_process);
}

void NET_getLinkStatus(char *strStatus) {
    const char *speed = (phyStatus & 2) ? "100M" : " 10M";
    const char *duplx = (phyStatus & 4) ? " FDX" : " HDX";
    const char *link = (phyStatus & 1) ? " ^" : " !";

    while(speed[0] != 0)
        *(strStatus++) = *(speed++);
    while(duplx[0] != 0)
        *(strStatus++) = *(duplx++);
    while(link[0] != 0)
        *(strStatus++) = *(link++);
    strStatus[0] = 0;
}

void NET_getMacAddress(char *strAddr) {
    uint8_t mac[6];
    getMAC(mac);
    for(int i = 0; i < 6; i++) {
        strAddr += toHex(mac[i], 2, '0', strAddr);
        *(strAddr++) = ':';
    }
    strAddr[-1] = 0;
}

void NET_poll() {
    while(!rxDesc[ptrRX].RDES0.OWN) {
        if(!rxDesc[ptrRX].RDES0.ES) {
            uint8_t *buffer = (uint8_t *) rxDesc[ptrRX].BUFF1;
            if(ETH_isARP(((struct FRAME_ETH *) buffer)->ethType)) {
                ARP_process(buffer, rxDesc[ptrRX].RDES0.FL);
            }
            else if(ETH_isIPv4(((struct FRAME_ETH *) buffer)->ethType)) {
                IPv4_process(buffer, rxDesc[ptrRX].RDES0.FL);
            }
        }
        rxDesc[ptrRX].RDES0.OWN = 1;
        ptrRX = (ptrRX + 1) & (RX_RING_SIZE-1);
    }

    // if link is up, poll ARP and DHCP state
    if(phyStatus & 1) {
        ARP_poll();
        DHCP_poll();
    }
}

int NET_getTxDesc() {
    if(txDesc[ptrTX].TDES0.OWN)
        return -1;

    int temp = ptrTX;
    ptrTX = (ptrTX + 1) & (TX_RING_SIZE-1);
    return temp;
}

uint8_t * NET_getTxBuff(int desc) {
    return (uint8_t *) txDesc[desc & (TX_RING_SIZE-1)].BUFF1;
}

void NET_transmit(int desc, int len) {
    // restrict transmission length
    if(len < 60) len = 60;
    if(len > TX_BUFF_SIZE) len = TX_BUFF_SIZE;
    // set transmission size
    txDesc[desc & (TX_RING_SIZE-1)].TDES1.TBS1 = len;
    // release descriptor
    txDesc[desc & (TX_RING_SIZE-1)].TDES0.OWN = 1;
    // wake TX DMA
    EMAC0.TXPOLLD = 1;
}
