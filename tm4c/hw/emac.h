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
        enum EMAC_PRELEN PRELEN: 2;
        unsigned RE: 1;
        unsigned TE: 1;
        unsigned DC: 1;
        enum EMAC_BL BL: 2;
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
        enum EMAC_IFG IFG: 3;
        unsigned JFEN: 1;
        unsigned : 1;
        unsigned JD: 1;
        unsigned WDDIS: 1;
        unsigned : 1;
        unsigned CST: 1;
        unsigned : 1;
        unsigned TWOKPEN: 1;
        enum EMAC_SADDR SADDR: 3;
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
    char _reserved_0[0x004];

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
    char _reserved_1[0x008];

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
    char _reserved_2[0x08C];

    // offset 0x0DC
    // Ethernet MAC Watchdog Timeout
    REGMAP_32 (, {
        unsigned WTO: 14;
        unsigned : 2;
        unsigned PWE: 1;
    }) WDOGTO;

    // reserved space
    char _reserved_3[0x010];

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
    char _reserved_4[0x004];

    // offset 0x118
    // Ethernet MAC Transmit Frame Count for Good and Bad Frames
    uint32_t TXCNTGB;

    // reserved space
    char _reserved_5[0x030];

    // offset 0x14C
    // Ethernet MAC Transmit Frame Count for Frames Transmitted after Single Collision
    uint32_t TXCNTSCOL;

    // offset 0x150
    // Ethernet MAC Transmit Frame Count for Frames Transmitted after Multiple Collisions
    uint32_t TXCNTMCOL;

    // reserved space
    char _reserved_6[0x010];

    // offset 0x164
    // Ethernet MAC Transmit Octet Count Good
    uint32_t TXOCTCNTG;

    // reserved space
    char _reserved_7[0x018];

    // offset 0x180
    // Ethernet MAC Receive Frame Count for Good and Bad Frames
    uint32_t RXCNTGB;

    // reserved space
    char _reserved_8[0x010];

    // offset 0x194
    // Ethernet MAC Receive Frame Count for CRC Error Frames
    uint32_t RXCNTCRCERR;

    // offset 0x198
    // Ethernet MAC Receive Frame Count for Alignment Error Frames
    uint32_t RXCNTALGNERR;

    // reserved space
    char _reserved_9[0x028];

    // offset 0x1C4
    // Ethernet MAC Receive Frame Count for Good Unicast Frames
    uint32_t RXCNTGUNI;

    // reserved space
    char _reserved_A[0x3BC];

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
    char _reserved_B[0x174];

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

    // reserved space
    char _reserved_C[0x8BC];

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

    char _reserved_E[0x034];

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

#endif //TM4C_EMAC_H
