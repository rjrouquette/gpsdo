# TM4C1249NCPDT Source Code

### Sections

- [Pin Assigments](#pin-assignments)
- [Support SNMP MIBs](#supported-snmp-mibs)
- [Hardware References](#hardware-references)
- [NTP References](#ntp-references)
- [SNMP References](#snmp-references)

## Pin Assignments

- PPS Timing Pins
  - PG0 (49) - ENOPPS (EMAC PPS Output)
  - PM6 (72) - T5CCP0 (Timer 5 Capture 0) tied to PG0
  - PM7 (71) - T5CCP0 (Timer 5 Capture 1) tied to GPS PPS
- GPS Data Pins
  - PJ0 (116) - U3RX (UART 3 RX) tied to GPS UART TX
  - PJ1 (117) - U3TX (UART 3 TX) tied to GPS UART RX
  - PP5 (106) - GPIO tied to GPS Reset (Active Low)


## Supported SNMP MIBs:


## Hardware References:
- [MSP432E401Y Datasheet](../docs/msp432e401y.pdf)
- [MSP432E401Y Manual](../docs/slau723a.pdf)
- [TM4C1249NCPDT Datasheet](../docs/tm4c1294ncpdt.pdf)
- [EK-TM4C1249XL Manual](../docs/EK-TM4C1249XL.pdf)
- [TM4C Template for Linux](https://github.com/shawn-dsilva/tm4c-linux-template)
- [TM4C-GCC](https://github.com/martinjaros/tm4c-gcc)
- [LM73 Datasheet](../docs/lm73.pdf)
- [SiT3521 Datasheet](../docs/SiT3521.pdf)

## NTP References:
- [RFC 5905: Network Time Protocol Version 4: Protocol and Algorithms Specification](https://www.ietf.org/rfc/rfc5905.html)

## SNMP References:
- [RFC 1155: Structure and Identification of Management Information for TCP/IP-based Internets](https://www.ietf.org/rfc/rfc1155.html)
- [RFC 1157: A Simple Network Management Protocol (SNMP)](https://www.ietf.org/rfc/rfc1157.html)
- [RFC 5907: Definitions of Managed Objects for Network Time Protocol Version 4 (NTPv4)](https://www.ietf.org/rfc/rfc5907.html)
- [RFC 7577: Definition of Managed Objects for Battery Monitoring](https://www.ietf.org/rfc/rfc7577.html)
- [RFC 8173: Precision Time Protocol Version 2 (PTPv2) Management Information Base](https://www.ietf.org/rfc/rfc8173.html)
- [SMI Network Management MGMT Codes](https://www.iana.org/assignments/smi-numbers/smi-numbers.xhtml#smi-numbers-2)
- [X.690 (ANS.1)](../docs/T-REC-X.690-202102-I.pdf)
