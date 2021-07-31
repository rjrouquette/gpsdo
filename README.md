# GPSDO PTP Master

Redesign of the [Radio Astronomy Master Clock](https://github.com/rjrouquette/radio_astronomy#master-clock) as an IEEE 1588 PTP Master.

- GPSDO
    - MSP430F5171
        - 16-bit high-resolution timer
            - internall PLL provides 8x base clock (200 MHz)
            - provides 5 ns resolution for offset measurement
        - PPS generation
        - GPS PPS capture
        - external PPS capture
            -high resolution comparison with generated PPS
        - DCXO control loop
            - age compensation
            - temperature compensation
            - drift measurement against GPS
    - DCXO for more precise tuning
        - 25 MHz
        - Si549 provides 116.4 ppt step size
        - I2C interface
    - Temperature sensor
        - same PCB thermal island with DCXO
        - allows for improved tracking and holdover accuracy
        - I2C interface
    - ublox MAX-8Q GPS
        - better timing accuracy than TESEO-LIV3R
        - I2C and UART interfaces
    - assembly as daughterboard allows for better isolation from thermal transients
- IEEE 1588 PTP Master
    - MSP432E401Y
        - integrated EMAC+PHY with PTP support
        - PTP PPS generation for synchronization with GPSDO
    - Uses 25MHz from GPSDO as PTP and PHY base clock
    - DHCP client
    - NTP server
    - HTTP status page
    - UART and Ethernet firmware updates
    - POE powered w/ battery backup
