/**
 * Ethernet MAC Register Mapping
 * @author Robert J. Rouquette
 * @date 2022-04-24
 */

#ifndef TM4C_EMAC_H
#define TM4C_EMAC_H

#include "register.h"

enum EMAC_PRELEN {
    EMAC_PRELEN_7BYTES = 0x0,
    EMAC_PRELEN_5BYTES = 0x1,
    EMAC_PRELEN_3BYTES = 0x2
};

enum EMAC_BL {
    EMAC_BL_10 = 0x0,
    EMAC_BL_8 = 0x1,
    EMAC_BL_4 = 0x2,
    EMAC_BL_1 = 0x3,
};

enum EMAC_IFG {
    EMAC_EFG_96BITS = 0x0,
    EMAC_EFG_88BITS = 0x1,
    EMAC_EFG_80BITS = 0x2,
    EMAC_EFG_72BITS = 0x3,
    EMAC_EFG_64BITS = 0x4,
    EMAC_EFG_56BITS = 0x5,
    EMAC_EFG_48BITS = 0x6,
    EMAC_EFG_40BITS = 0x7
};

enum EMAC_SADDR {
    EMAC_SADDR_INS0 = 0x02,
    EMAC_SADDR_REP0 = 0x03,
    EMAC_SADDR_INS1 = 0x06,
    EMAC_SADDR_REP1 = 0x07,
};

struct EMAC_MACADDR {
    REGMAP_32 (, {
        unsigned ADDR: 16;
        unsigned : 8;
        unsigned MBC: 6;
        unsigned SA: 1;
        unsigned AE: 1;
    }) HI;
    uint32_t LO;
};
_Static_assert(sizeof(struct EMAC_MACADDR) == 8, "EMAC_MACADDR must be 8 bytes");

PAGE_MAP (EMAC_MAP, {
    // offset 0x000
    // Ethernet MAC Configuration
    REGMAP_32 (, {
        unsigned PRELEN: 2;
        unsigned RE: 1;
        unsigned TE: 1;
        unsigned DC: 1;
        unsigned BL: 2;
        unsigned ACS: 1;
        unsigned : 1;
        unsigned DR: 1;
        unsigned IPC: 1;
        unsigned DUPM: 1;
        unsigned LOOPBM: 1;
        unsigned DRO: 1;
        unsigned FES: 1;
        unsigned PS: 1;
        unsigned DISCRS: 1;
        unsigned IFG: 3;
        unsigned JFEN: 1;
        unsigned : 1;
        unsigned JD: 1;
        unsigned WDDIS: 1;
        unsigned : 1;
        unsigned CST: 1;
        unsigned : 1;
        unsigned TWOKPEN: 1;
        unsigned SADDR: 3;
        unsigned : 1;
    }) CFG;

    // offset 0x004
    // Ethernet MAC Frame Filter
    REGMAP_32 (, {
        unsigned PR: 1;
        unsigned HUC: 1;
        unsigned HMC: 1;
        unsigned DAIF: 1;
        unsigned PM: 1;
        unsigned DBF: 1;
        unsigned PCF: 2;
        unsigned SAIF: 1;
        unsigned SAF: 1;
        unsigned HPF: 1;
        unsigned : 5;
        unsigned VTFE: 1;
        unsigned : 14;
        unsigned RA: 1;
    }) FRAMEFLTR;

    // offset 0x008
    // Ethernet MAC Hash Table High
    uint32_t HASHTBLH;

    // offset 0x00C
    // Ethernet MAC Hash Table Low
    uint32_t HASHTBLL;

    // offset 0x010
    // Ethernet MAC MII Address
    REGMAP_32 (, {
        unsigned MIB: 1;
        unsigned MIW: 1;
        unsigned CR: 4;
        unsigned MII: 5;
        unsigned PLA: 5;
    }) MIIADDR;

    // offset 0x014
    // Ethernet MAC MII Data Register
    REGMAP_32 (, {
        unsigned DATA: 16;
    }) MIIDATA;

    // offset 0x018
    // Ethernet MAC Flow Control
    REGMAP_32 (, {
        unsigned FCBBPA: 1;
        unsigned TFE: 1;
        unsigned RFE: 1;
        unsigned UP: 1;
        unsigned : 3;
        unsigned DZQP: 1;
        unsigned : 8;
        unsigned PT: 16;
    }) FLOWCTL;

    // offset 0x01C
    // Ethernet MAC VLAN Tag
    REGMAP_32 (, {
        unsigned VL: 16;
        unsigned ETV: 1;
        unsigned VTIM: 1;
        unsigned ESVL: 1;
        unsigned VTHM: 1;
    }) VLANTG;

    // reserved space
    char _reserved_00[0x004];

    // offset 0x024
    // Ethernet MAC Status
    REGMAP_32 (, {
        unsigned RPE: 1;
        unsigned RFCFC: 2;
        unsigned : 1;
        unsigned RWC: 1;
        unsigned RRC: 2;
        unsigned : 1;
        unsigned RXF: 1;
        unsigned : 6;
        unsigned TPE: 1;
        unsigned TFC: 2;
        unsigned TXPAUSED: 1;
        unsigned TRC: 2;
        unsigned TWC: 1;
        unsigned : 1;
        unsigned TXFE: 1;
        unsigned TXFF: 1;
    }) STATUS;

    // offset 0x028
    // Ethernet MAC Remote Wake-Up Frame Filter
    uint32_t RWUFF;

    // offset 0x02C
    // Ethernet MAC PMT Control and Status Register
    REGMAP_32 (, {
        unsigned PWRDWN: 1;
        unsigned MGKPKTEN: 1;
        unsigned WUPFREN: 1;
        unsigned : 2;
        unsigned MGKPRX: 1;
        unsigned WUPRX: 1;
        unsigned : 2;
        unsigned GLBLUCAST: 1;
        unsigned : 14;
        unsigned RWKPTR: 3;
        unsigned : 4;
        unsigned WUPFRRST: 1;
    }) PMTCTLSTAT;

    // reserved space
    char _reserved_01[0x008];

    // offset 0x038
    // Ethernet MAC Raw Interrupt Status
    REGMAP_32 (, {
        unsigned : 3;
        unsigned PMT: 1;
        unsigned MMC: 1;
        unsigned MMCRX: 1;
        unsigned MMCTX: 1;
        unsigned : 2;
        unsigned TS : 1;
    }) RIS;

    // offset 0x03C
    // Ethernet MAC Interrupt Mask
    REGMAP_32 (, {
        unsigned : 3;
        unsigned PMT: 1;
        unsigned : 5;
        unsigned TS : 1;
    }) IM;

    // offset 0x040
    struct EMAC_MACADDR ADDR0;
    struct EMAC_MACADDR ADDR1;
    struct EMAC_MACADDR ADDR2;
    struct EMAC_MACADDR ADDR3;

    // reserved space
    char _reserved_02[0x08C];

    // offset 0x0DC
    // Ethernet MAC Watchdog Timeout
    REGMAP_32 (, {
        unsigned WTO: 14;
        unsigned : 2;
        unsigned PWE: 1;
    }) WDOGTO;

    // reserved space
    char _reserved_03[0x010];

    // offset 0x100
    // Ethernet MAC MMC Control
    REGMAP_32 (, {
        unsigned CNTRST: 1;
        unsigned CNTSTPRO: 1;
        unsigned RSTONRD: 1;
        unsigned CNTFREEZ: 1;
        unsigned CNTPRST: 1;
        unsigned CNTPRSTLVL: 1;
        unsigned : 2;
        unsigned UCDBC: 1;
    }) MMCCTRL;

    // offset 0x104
    // Ethernet MAC MMC Receive Raw Interrupt Status
    REGMAP_32 (, {
        unsigned GBF: 1;
        unsigned : 4;
        unsigned CRCERR: 1;
        unsigned ALGNERR: 1;
        unsigned : 10;
        unsigned UCGF : 1;
    }) MMCRXRIS;

    // offset 0x108
    // Ethernet MAC MMC Transmit Raw Interrupt Status
    REGMAP_32 (, {
        unsigned : 1;
        unsigned GBF: 1;
        unsigned : 14;
        unsigned SCOLLGF: 1;
        unsigned MCOLLGF: 1;
        unsigned : 4;
        unsigned OCTCNT: 1;
    }) MMCTXRIS;

    // offset 0x10C
    // Ethernet MAC MMC Receive Interrupt Mask
    REGMAP_32 (, {
        unsigned GBF: 1;
        unsigned : 4;
        unsigned CRCERR: 1;
        unsigned ALGNERR: 1;
        unsigned : 10;
        unsigned UCGF: 1;
    }) MMCRXIM;

    // offset 0x110
    // Ethernet MAC MMC Transmit Interrupt Mask
    REGMAP_32 (, {
        unsigned : 1;
        unsigned GBF: 1;
        unsigned : 14;
        unsigned SCOLLGF: 1;
        unsigned MCOLLGF: 1;
        unsigned : 4;
        unsigned OCTCNT: 1;
    }) MMCTXIM;

    // reserved space
    char _reserved_04[0x004];

    // offset 0x118
    // Ethernet MAC Transmit Frame Count for Good and Bad Frames
    uint32_t TXCNTGB;

    // reserved space
    char _reserved_05[0x030];

    // offset 0x14C
    // Ethernet MAC Transmit Frame Count for Frames Transmitted after Single Collision
    uint32_t TXCNTSCOL;

    // offset 0x150
    // Ethernet MAC Transmit Frame Count for Frames Transmitted after Multiple Collisions
    uint32_t TXCNTMCOL;

    // reserved space
    char _reserved_06[0x010];

    // offset 0x164
    // Ethernet MAC Transmit Octet Count Good
    uint32_t TXOCTCNTG;

    // reserved space
    char _reserved_07[0x018];

    // offset 0x180
    // Ethernet MAC Receive Frame Count for Good and Bad Frames
    uint32_t RXCNTGB;

    // reserved space
    char _reserved_08[0x010];

    // offset 0x194
    // Ethernet MAC Receive Frame Count for CRC Error Frames
    uint32_t RXCNTCRCERR;

    // offset 0x198
    // Ethernet MAC Receive Frame Count for Alignment Error Frames
    uint32_t RXCNTALGNERR;

    // reserved space
    char _reserved_09[0x028];

    // offset 0x1C4
    // Ethernet MAC Receive Frame Count for Good Unicast Frames
    uint32_t RXCNTGUNI;

    // reserved space
    char _reserved_0A[0x3BC];

    // offset 0x584
    // Ethernet MAC VLAN Tag Inclusion or Replacement
    REGMAP_32 (, {
        unsigned VLT: 16;
        unsigned VLC: 2;
        unsigned VLP: 1;
        unsigned CSVL: 1;
    }) VLNINCREP;

    // offset 0x588
    // Ethernet MAC VLAN Hash Table
    REGMAP_32 (, {
        unsigned VLHT: 16;
    }) VLANHASH;

    // reserved space
    char _reserved_0B[0x174];

    // offset 0x700
    // Ethernet MAC Timestamp Control
    REGMAP_32 (, {
        unsigned TSEN: 1;
        unsigned TSFCUPDT: 1;
        unsigned TSINIT: 1;
        unsigned TSUPDT: 1;
        unsigned INTTRIG: 1;
        unsigned ADDREGUP: 1;
        unsigned : 2;
        unsigned ALLF: 1;
        unsigned DGTLBIN: 1;
        unsigned PTPVER2: 1;
        unsigned PTPETH: 1;
        unsigned PTPIPV6: 1;
        unsigned PTPIPV4: 1;
        unsigned TSEVNT: 1;
        unsigned TSMAST: 1;
        unsigned SELPTP: 1;
        unsigned PTPFLTR: 1;
    }) TIMSTCTRL;

    // offset 0x704
    // Ethernet MAC Sub-Second Increment
    REGMAP_32(, {
        unsigned SSINC: 8;
    }) SUBSECINC;

    // offset 0x708
    // Ethernet MAC System Time - Seconds
    uint32_t TIMSEC;

    // offset 0x70C
    // Ethernet MAC System Time - Nanoseconds
    uint32_t TIMNANO;

    // offset 0x710
    // Ethernet MAC System Time - Seconds Update
    uint32_t TIMSECU;

    // offset 0x714
    // Ethernet MAC System Time - Nanoseconds Update
    REGMAP_32(, {
        unsigned VALUE: 31;
        unsigned NEG: 1;
    }) TIMNANOU;

    // offset 0x718
    // Ethernet MAC Timestamp Addend
    uint32_t TIMADD;

    // offset 0x71C
    // Ethernet MAC Target Time Seconds
    uint32_t TARGSEC;

    // offset 0x720
    // Ethernet MAC Target Time Nanoseconds
    uint32_t TARGNANO;

    // offset 0x724
    // Ethernet MAC System Time-Higher Word Seconds
    REGMAP_32(, {
        unsigned TSHWR: 16;
    }) HWORDSEC;

    // offset 0x728
    // Ethernet MAC Timestamp Status
    REGMAP_32(, {
        unsigned TSSOVF: 1;
        unsigned TSTARGT: 1;
    }) TIMSTAT;

    // offset 0x72C
    // Ethernet MAC PPS Control
    REGMAP_32(, {
        unsigned PPSCTRL: 4;
        unsigned PPSEN0: 1;
        unsigned TRGMODS0: 2;
    }) PPSCTRL;

    // reserved space
    char _reserved_0C[0x030];

    // offset 0x760
    // Ethernet MAC PPS0 Interval
    uint32_t PPS0INTVL;

    // offset 0x764
    // Ethernet MAC PPS0 Interval
    uint32_t PPS0WIDTH;

    // reserved space
    char _reserved_0D[0x498];

    // offset 0xC00
    // Ethernet MAC DMA Bus Mode
    REGMAP_32(, {
        unsigned SWR: 1;
        unsigned DA: 1;
        unsigned DSL: 5;
        unsigned ATDS: 1;
        unsigned PBL: 6;
        unsigned PR: 2;
        unsigned FB: 1;
        unsigned RPBL: 6;
        unsigned USP: 1;
        unsigned PBLx8: 1;
        unsigned AAL: 1;
        unsigned MB: 1;
        unsigned TXPR: 1;
        unsigned : 3;
        unsigned RIB: 1;
    }) DMABUSMOD;

    // offset 0xC04
    // Ethernet MAC Transmit Poll Demand
    uint32_t TXPOLLD;

    // offset 0xC08
    // Ethernet MAC Receive Poll Demand
    uint32_t RXPOLLD;

    // offset 0xC0C
    // Ethernet MAC Receive Descriptor List Address
    uint32_t RXDLADDR;

    // offset 0xC10
    // Ethernet MAC Transmit Descriptor List Address
    uint32_t TXDLADDR;

    // offset 0xC14
    // Ethernet MAC DMA Interrupt Status
    REGMAP_32(, {
        unsigned TI: 1;
        unsigned TPS: 1;
        unsigned TU: 1;
        unsigned TJT: 1;
        unsigned OVF: 1;
        unsigned UNF: 1;
        unsigned RI: 1;
        unsigned RU: 1;
        unsigned RPS: 1;
        unsigned RWT: 1;
        unsigned ETI: 1;
        unsigned : 2;
        unsigned FBI: 1;
        unsigned ERI: 1;
        unsigned AIS: 1;
        unsigned NIS: 1;
        unsigned RS: 3;
        unsigned TS: 3;
        unsigned AE: 3;
        unsigned : 2;
        unsigned MMC: 1;
        unsigned PMT: 1;
        unsigned TT: 1;
    }) DMARIS;

    // offset 0xC18
    // Ethernet MAC DMA Operation Mode
    REGMAP_32(, {
        unsigned : 1;
        unsigned SR: 1;
        unsigned OSF: 1;
        unsigned RTC: 2;
        unsigned DGF: 1;
        unsigned FUF: 1;
        unsigned FEF: 1;
        unsigned : 5;
        unsigned ST: 1;
        unsigned TTC: 3;
        unsigned : 3;
        unsigned FTF: 1;
        unsigned TSF: 1;
        unsigned : 2;
        unsigned DFF: 1;
        unsigned RSF: 1;
        unsigned DT: 1;
    }) DMAOPMODE;

    // offset 0xC1C
    // Ethernet MAC DMA Interrupt Mask Register
    REGMAP_32(, {
        unsigned TIE: 1;
        unsigned TSE: 1;
        unsigned TUE: 1;
        unsigned TJE: 1;
        unsigned OVE: 1;
        unsigned UNE: 1;
        unsigned RIE: 1;
        unsigned RUE: 1;
        unsigned RSE: 1;
        unsigned RWE: 1;
        unsigned ETE: 1;
        unsigned : 2;
        unsigned FBE: 1;
        unsigned ERE: 1;
        unsigned AIE: 1;
        unsigned NIE: 1;
    }) DMAIM;

    // offset 0xC20
    // Ethernet MAC Missed Frame and Buffer
    REGMAP_32(, {
        unsigned MISFRMCNT: 16;
        unsigned MISCNTOVF: 1;
        unsigned OVFFRMCNT: 11;
        unsigned OVFCNTOVF: 1;
    }) MFBOC;

    // offset 0xC24
    // Ethernet MAC Receive Interrupt Watchdog Timer
    REGMAP_32(, {
        unsigned RIWT: 8;
    }) RXINTWDT;

    // reserved space
    char _reserved_0E[0x020];

    // offset 0xC48
    // Ethernet MAC Current Host Transmit Descriptor
    uint32_t HOSTXDESC;

    // offset 0xC4C
    // Ethernet MAC Current Host Receive Descriptor
    uint32_t HOSRXDESC;

    // offset 0xC50
    // Ethernet MAC Current Host Transmit Buffer Address
    uint32_t HOSTXBA;

    // offset 0xC54
    // Ethernet MAC Current Host Receive Buffer Address
    uint32_t HOSRXBA;

    // reserved space
    char _reserved_0F[0x368];

    // offset 0xFC0
    // Ethernet MAC Peripheral Property Register
    REGMAP_32 (, {
        unsigned PHYTYPE: 3;
        unsigned : 5;
        unsigned MACTYPE: 3;
    }) PP;

    // offset 0xFC4
    // Ethernet MAC Peripheral Configuration Register
    REGMAP_32(, {
        unsigned PHYHOLD: 1;
        unsigned ANMODE: 2;
        unsigned ANEN: 1;
        unsigned FASTANSEL: 2;
        unsigned FASTEN: 1;
        unsigned EXTFD: 1;
        unsigned FASTLUPD: 1;
        unsigned FASTRXDV: 1;
        unsigned MDIXEN: 1;
        unsigned FASTMDIX: 1;
        unsigned RBSTMDIX: 1;
        unsigned MDISWAP: 1;
        unsigned POLSWAP: 1;
        unsigned FASTLDMODE: 5;
        unsigned TDRRUN: 1;
        unsigned LRR: 1;
        unsigned ISOMILL: 1;
        unsigned RXERRIDLE: 1;
        unsigned NIBDETDIS: 1;
        unsigned DIGRESTART: 1;
        unsigned : 2;
        unsigned PINTFS: 3;
        unsigned PHYEXT: 1;
    }) PC;

    // offset 0xFC8
    // Ethernet MAC Clock Configuration Register
    REGMAP_32(, {
        unsigned : 17;
        unsigned POL: 1;
        unsigned PTPCEN: 1;
    }) CC;

    char _reserved_10[0x004];

    struct {
        // offset 0xFD0
        // Ethernet PHY Raw Interrupt Status
        REGMAP_32(, {
            unsigned INT: 1;
        }) RIS;

        // offset 0xFD4
        // Ethernet PHY Interrupt Mask
        REGMAP_32(, {
            unsigned INT: 1;
        }) IM;

        // offset 0xFD8
        // Ethernet PHY Masked Interrupt Status and Clear
        REGMAP_32(, {
            unsigned INT: 1;
        }) MIS;
    } PHY;

    char _reserved_11[0x024];
})

#define EMAC0   (*(volatile struct EMAC_MAP *)0x400EC000)

REGMAP_32 (RCGCEMAC_MAP, {
    unsigned EN0: 1;
});
#define RCGCEMAC (*(volatile union RCGCEMAC_MAP *)0x400FE69C)

REGMAP_32 (PCEMAC_MAP, {
    unsigned EN0: 1;
});
#define PCEMAC (*(volatile union PCEMAC_MAP *)0x400FE99C)

REGMAP_32 (PREMAC_MAP, {
    unsigned RDY0: 1;
});
#define PREMAC (*(volatile union PREMAC_MAP *)0x400FEA9C)

REGMAP_32 (RCGCEPHY_MAP, {
    unsigned EN0: 1;
});
#define RCGCEPHY (*(volatile union RCGCEPHY_MAP *)0x400FE630)

REGMAP_32 (PCEPHY_MAP, {
    unsigned EN0: 1;
});
#define PCEPHY (*(volatile union PCEPHY_MAP *)0x400FE930)

REGMAP_32 (PREPHY_MAP, {
    unsigned RDY0: 1;
});
#define PREPHY (*(volatile union PREPHY_MAP *)0x400FEA30)

struct EMAC_RX_DESC {
    REGMAP_32(, {
        unsigned ESA: 1;
        unsigned CE: 1;
        unsigned DE: 1;
        unsigned RE: 1;
        unsigned RWT: 1;
        unsigned FT: 1;
        unsigned LC: 1;
        unsigned TAGF: 1;
        unsigned LS: 1;
        unsigned FS: 1;
        unsigned VLAN: 1;
        unsigned OE: 1;
        unsigned LE: 1;
        unsigned SAF: 1;
        unsigned TRUNC: 1;
        unsigned ES: 1;
        unsigned FL: 14;
        unsigned AFM: 1;
        unsigned OWN: 1;
    }) RDES0;

    REGMAP_32(, {
        unsigned RBS1: 13;
        unsigned : 1;
        unsigned RCH: 1;
        unsigned RER: 1;
        unsigned RBS2: 13;
        unsigned : 2;
        unsigned DIC: 1;
    }) RDES1;

    uint32_t BUFF1;
    uint32_t BUFF2;
    REGMAP_32(, {
        unsigned IPPT: 3;
        unsigned IPHE: 1;
        unsigned IPPE: 1;
        unsigned IPCB: 1;
        unsigned IPv4: 1;
        unsigned IPv6: 1;
        unsigned PTPMT: 4;
        unsigned PTPFT: 1;
        unsigned PTPv2: 1;
        unsigned TD: 1;
    }) RDES4;
    uint32_t RDES5;
    uint32_t RTSL;
    uint32_t RTSH;
};
_Static_assert(sizeof(struct EMAC_RX_DESC) == 32, "EMAC_RX_DESC must be 32 bytes");

struct EMAC_TX_DESC {
    REGMAP_32(, {
        unsigned DB: 1;
        unsigned UF: 1;
        unsigned ED: 1;
        unsigned CC: 4;
        unsigned VF: 1;
        unsigned ECOL: 1;
        unsigned LCOL: 1;
        unsigned NCAR: 1;
        unsigned LCAR: 1;
        unsigned IPE: 1;
        unsigned FF: 1;
        unsigned JT: 1;
        unsigned ES: 1;
        unsigned IHE: 1;
        unsigned TTSS: 1;
        unsigned VLIC: 2;
        unsigned TCH: 1;
        unsigned TER: 1;
        unsigned CIC: 2;
        unsigned CRCR: 1;
        unsigned TTSE: 1;
        unsigned DP: 1;
        unsigned DC: 1;
        unsigned FS: 1;
        unsigned LS: 1;
        unsigned IC: 1;
        unsigned OWN: 1;
    }) TDES0;

    REGMAP_32(, {
        unsigned TBS1: 13;
        unsigned : 3;
        unsigned TBS2: 13;
        unsigned SAIC: 3;
    }) TDES1;

    uint32_t BUFF1;
    uint32_t BUFF2;
    uint32_t TDES4;
    uint32_t TDES5;
    uint32_t TTSL;
    uint32_t TTSH;
};
_Static_assert(sizeof(struct EMAC_TX_DESC) == 32, "EMAC_TX_DESC must be 32 bytes");

#define MII_ADDR_EPHYBMCR     (0x00u)   // Ethernet PHY Basic Mode Control - MR0
#define MII_ADDR_EPHYBMSR     (0x01u)   // Ethernet PHY Basic Mode Status - MR1
#define MII_ADDR_EPHYID1      (0x02u)   // Ethernet PHY Identifier Register 1 - MR2
#define MII_ADDR_EPHYID2      (0x03u)   // Ethernet PHY Identifier Register 2 - MR3
#define MII_ADDR_EPHYANA      (0x04u)   // Ethernet PHY Auto-Negotiation Advertisement - MR4
#define MII_ADDR_EPHYANLPA    (0x05u)   // Ethernet PHY Auto-Negotiation Link Partner Ability - MR5
#define MII_ADDR_EPHYANER     (0x06u)   // Ethernet PHY Auto-Negotiation Expansion - MR6
#define MII_ADDR_EPHYANNPTR   (0x07u)   // Ethernet PHY Auto-Negotiation Next Page TX - MR7
#define MII_ADDR_EPHYANLNPTR  (0x08u)   // Ethernet PHY Auto-Negotiation Link Partner Ability Next Page - MR8
#define MII_ADDR_EPHYCFG1     (0x09u)   // Ethernet PHY Configuration 1 - MR9
#define MII_ADDR_EPHYCFG2     (0x0Au)   // Ethernet PHY Configuration 2 - MR10
#define MII_ADDR_EPHYCFG3     (0x0Bu)   // Ethernet PHY Configuration 3 - MR11
#define MII_ADDR_EPHYREGCTL   (0x0Du)   // Ethernet PHY Register Control - MR13
#define MII_ADDR_EPHYADDAR    (0x0Eu)   // Ethernet PHY Address or Data - MR14
#define MII_ADDR_EPHYSTS      (0x10u)   // Ethernet PHY Status - MR16
#define MII_ADDR_EPHYSCR      (0x11u)   // Ethernet PHY Specific Control- MR17
#define MII_ADDR_EPHYMISR1    (0x12u)   // Ethernet PHY MII Interrupt Status 1 - MR18
#define MII_ADDR_EPHYMISR2    (0x13u)   // Ethernet PHY MII Interrupt Status 2 - MR19
#define MII_ADDR_EPHYFCSCR    (0x14u)   // Ethernet PHY False Carrier Sense Counter - MR20
#define MII_ADDR_EPHYRXERCNT  (0x15u)   // Ethernet PHY Receive Error Count - MR21
#define MII_ADDR_EPHYBISTCR   (0x16u)   // Ethernet PHY BIST Control - MR22
#define MII_ADDR_EPHYLEDCR    (0x18u)   // Ethernet PHY LED Control - MR24
#define MII_ADDR_EPHYCTL      (0x19u)   // Ethernet PHY Control - MR25
#define MII_ADDR_EPHY10BTSC   (0x1Au)   // Ethernet PHY 10Base-T Status/Control - MR26
#define MII_ADDR_EPHYBICSR1   (0x1Bu)   // Ethernet PHY BIST Control and Status 1 - MR27
#define MII_ADDR_EPHYBICSR2   (0x1Cu)   // Ethernet PHY BIST Control and Status 2 - MR28
#define MII_ADDR_EPHYCDCR     (0x1Eu)   // Ethernet PHY BIST Control and Status 2 - MR28
#define MII_ADDR_EPHYCDCR     (0x1Eu)   // Ethernet PHY Cable Diagnostic Control - MR30
#define MII_ADDR_EPHYRCR      (0x1Fu)   // Ethernet PHY Reset Control - MR31
#define MII_ADDR_EPHYLEDCFG   (0x25u)   // Ethernet PHY LED Configuration - MR37

uint16_t EMAC_MII_Read(volatile struct EMAC_MAP *emac, uint8_t address);

void EMAC_MII_Write(volatile struct EMAC_MAP *emac, uint8_t address, uint16_t value);

#endif //TM4C_EMAC_H
