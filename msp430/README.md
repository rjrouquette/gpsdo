# GPSDO (MSP430F5171)

Documentation for GPSDO microntroller.

---
## DCXO Feedback Loop

The feedback loop combines the XO aging offset, temperature coefficient, and PPS PI controller to compute the frequency offset for the DCXO.

```
FBt = Ct * temp + C0(temp)
FBg = Cp * Epps + Ci * Acc(Epps)

FB = FBt + FBg
```

The temperature coefficient is computed using linear interpolation between the measured coefficient at different temperature increments.

The XO aging offset is derived from the residuals of the temperature coefficient estimation.

---
## I2C Bus

The ublox MAX-8Q DDC interface consumes the entier I2C address space. An LTC4317 is used to multiplex the I2C bus and prevent address collision. Multiplexing is performed via the ENABLE[1:2] pins on the LTC4317.

- Bus 1
    - Si549 25 MHz DCXO (0x55) [549CAAC000111BBG]
    - TMP117 Temperature Sensor (0x49) [TMP117MAIDRVT]
    - 47L64 64k-bit EERAM x2 (0xA1, 0xA3) [47L64-I/SN]
- Bus 2
    - ublox MAX-8Q (0x00-0xFF) [MAX-8Q-0]

---
## Si549 DCXO

Register values for 25 MHz:
- LSDIV = 0
- HSDIV = 432
- FBDIV = 70.77326343
- REG[23:31] = 0xB0 0x01 0xA7 0x97 0xF4 0xC5 0x46 0x00

I2C Init:
```
S:0x55:W:A :0xFF:A :0x00:A:P
S:0x55:W:A :0x45:A :0x00:A:P
S:0x55:W:A :0x1F:A :0x00:A:P
S:0x55:W:A :0x17:A :0xB0:A :0x01:AP
S:0x55:W:A :0x1A:A :0xA7:A :0x97:A :0xF4:A :0xC5:A :0x46:A :0x00:A:P
S:0x55:W:A :0x07:A :0x08:A:P
S:0x55:W:A :0x1F:A :0x01:A:P
```

I2C FBDIV:
```
S:0x55:W:A :0x1A:A :[0]:A :[1]:A :[2]:A :[3]:A :[4]:A :[5]:A:P
```

I2C ADPLL:
```
S:0x55:W:A :0xE7:A :[0]:A :[1]:A :[2]:A:P
```

---
## TMP117 Temperature Sensor

Configure for 64 samples per second and 64 sample averaging. Produces one result every second.

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
## 47L64 EERAM w/ EEPROM

Two 8kB banks:
- 0xA1 - Configuration Data
- 0xA3 - Temperature Compensation Data

I2C Read:
```
S:CSA:W:A :ADDRH:A :ADDRL:A
S:CSA:R:A :DATA[0]:A .. :DATA[N]:A:P
```

I2C Write:
```
S:CSA:W:A :ADDRH:A :ADDRL:A :DATA[0]:A .. :DATA[N]:A:P
```

---
## ublox MAX-8Q

TODO: add ublox DDC documentation
