# GPSDO PTP Master

Redesign of the [Radio Astronomy Master Clock](https://github.com/rjrouquette/radio_astronomy#master-clock) as an IEEE 1588 PTP Master.

## IEEE 1588 PTP Master
- ublox MAX-8Q GPS
- MSP432E401Y
    - integrated EMAC+PHY with PTP support
    - PTP PPS generation for synchronization with GPSDO
- Software discipline of XO frequency and offset
- PPS and 1 kHz reference outputs
- POE powered w/ battery backup
- DHCP client
- NTP server
- PTP server
- SNMP server
- UDP status endpoints
