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
    char _reserved_0[0x008];

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
    char _reserved_2[0x07C];

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

    // reserved space
    char _reserved_02[0x8];
    // offset 0xFD0
    uint32_t PeriphID4;
    uint32_t PeriphID5;
    uint32_t PeriphID6;
    uint32_t PeriphID7;
    uint32_t PeriphID0;
    uint32_t PeriphID1;
    uint32_t PeriphID2;
    uint32_t PeriphID3;
    uint32_t PCellID0;
    uint32_t PCellID1;
    uint32_t PCellID2;
    uint32_t PCellID3;
})

#define EMAC0   (*(volatile struct ADC_MAP *) 0x400EC000)

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
