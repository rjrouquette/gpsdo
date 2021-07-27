# GPSDO (MSP430F5171)

Documentation for GPSDO microntroller.

---
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

---
## Si549

TODO: add documention for clock trimming

---
## Si7051

Poll at 64 Hz and average 64 samples to produce one result every second.

I2C Read:
```
S:0x40:W:A :0xF3:A:P
wait l2 ms
S:0x40:R:A :MSB:A :LSB:A :CHKSUM:N:P
```

tempCelsius = ((175.72 * code) / 65536) - 46.85

---
## TMP117

Configure for 64 samples per second and 64 sample averaging.  Produces one result every second.

I2C Init:
```
S:0x49:W:A :0x01:A :0x00:A :0x60:A:P
```

I2C Read:
```
S:0x49:W:A :0x00:A RS:0x49:R:A :MSB:A :LSB:N:P
```

tempCelsius = code * 0.0078125

---
## ublox MAX-8Q
