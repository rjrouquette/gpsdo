# TM4C1249NCPDT Source Code


Supported SNMP MIB Requests:
- 1.3.6.1.2.1.197.1.1 = NTP Info (Get)
  - 1.3.6.1.2.1.197.1.1.1 = NTP Software Name
  - 1.3.6.1.2.1.197.1.1.2 = NTP Software Version
  - 1.3.6.1.2.1.197.1.1.3 = NTP Software Vendor
  - 1.3.6.1.2.1.197.1.1.4 = NTP System Type
  - 1.3.6.1.2.1.197.1.1.5 = NTP Time Resolution
  - 1.3.6.1.2.1.197.1.1.6 = NTP Time Precision
  - 1.3.6.1.2.1.197.1.1.7 = NTP Time Distance
- 1.3.6.1.2.1.197.1.2 NTP Status (Get)
  - 1.3.6.1.2.1.197.1.2.1 NTP Current Mode
    - 2 = Not Synchronized
    - 5 = GPS Synchronized
  - 1.3.6.1.2.1.197.1.2.2 NTP Stratum
  - 1.3.6.1.2.1.197.1.2.3 NTP Active Reference ID
  - 1.3.6.1.2.1.197.1.2.4 NTP Active Reference Name
  - 1.3.6.1.2.1.197.1.2.5 NTP Active Reference Offset
  - 1.3.6.1.2.1.197.1.2.6 NTP Number of References
  - 1.3.6.1.2.1.197.1.2.7 NTP Root Dispersion
  - 1.3.6.1.2.1.197.1.2.8 NTP Uptime (in seconds)
  - 1.3.6.1.2.1.197.1.2.9 NTP Date (128-bit format)


Hardware References:
- [TM4C1249NCPDT Datasheet](../docs/tm4c1294ncpdt.pdf)
- [EK-TM4C1249XL Manual](../docs/EK-TM4C1249XL.pdf)
- [TM4C Template for Linux](https://github.com/shawn-dsilva/tm4c-linux-template)
- [TM4C-GCC](https://github.com/martinjaros/tm4c-gcc)
- [LM73 Datasheet](../docs/lm73.pdf)
- [SiT3521 Datasheet](../docs/SiT3521.pdf)

NTP References:
- [RFC 5905: Network Time Protocol Version 4: Protocol and Algorithms Specification](https://www.ietf.org/rfc/rfc5905.html)

SNMP References:
- [RFC 1155: Structure and Identification of Management Information for TCP/IP-based Internets](https://www.ietf.org/rfc/rfc1155.html)
- [RFC 1157: A Simple Network Management Protocol (SNMP)](https://www.ietf.org/rfc/rfc1157.html)
- [RFC 5907: Definitions of Managed Objects for Network Time Protocol Version 4 (NTPv4)](https://www.ietf.org/rfc/rfc5907.html)
- [RFC 7577: Definition of Managed Objects for Battery Monitoring](https://www.ietf.org/rfc/rfc7577.html)
- [RFC 8173: Precision Time Protocol Version 2 (PTPv2) Management Information Base](https://www.ietf.org/rfc/rfc8173.html)
- [SMI Network Management MGMT Codes](https://www.iana.org/assignments/smi-numbers/smi-numbers.xhtml#smi-numbers-2)
- [X.690 (ANS.1)](../docs/T-REC-X.690-202102-I.pdf)
