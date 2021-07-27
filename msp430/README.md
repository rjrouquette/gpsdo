# GPSDO (MSP430F5171)

Documentation for GPSDO microntroller.


## I2C Bus

The ublox MAX-8Q DDC interface consumes the entier I2C address space.  An
LTC4317 is used to multiplex the I2C bus, and prevent address collision.
Multiplexing is performed via the ENABLE[1:2] pins on the LTC4317.

- Bus 1
    - Si549 25 MHz DCXO (0x55) [549CAAC000111BBG]
    - Si7051 Temperature Sensor (0x40) [SI7051-A20-IM]
        - may substitute with TMP117 (0x49) [TMP117AIDRVR ]
- Bus 2
    - ublox MAX-8Q (0x00-0xFF) [MAX-8Q-0]
