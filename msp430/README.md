# GPSDO (MSP430F5171)

Documentation for GPSDO microntroller.

---
## MSP430F5171

Specifications:
- CPU: MSP430
- CLK: 25 MHz
- Flash: 32 kB
- SRAM: 2 kB
- ADC: None
- HW Mult: MPY32 (32-bit)

---
## DCXO Feedback Loop

The feedback loop combines the temperature coefficient, temperature offset, and PPS PID controller to derive the frequency offset for the DCXO.

Please see the fixed-point-math test [README](../fpm-test/README.md) for more details about the temperature compensation method.

---
## I2C Bus

- (0x42) ublox MAX-8Q [MAX-8Q-0]
- (0x49) TMP117 Temperature Sensor [TMP117MAIDRVT]
- (0x51) 47L64 64k-bit EERAM [47L64-I/SN]
- (0x55) Si549 25 MHz DCXO [549CAAC000111BBG]

---
## Si549 DCXO

Register values for 25 MHz:
- LSDIV = 0
- HSDIV = 432
- FBDIV = 70.77326343
- REG[23:24] = 0xB0 0x01
- REG[26:31] = 0xA7 0x97 0xF4 0xC5 0x46 0x00

I2C Init:
```
S:0x55:W:A :0xFF:A :0x00:A:P
S:0x55:W:A :0x45:A :0x00:A:P
S:0x55:W:A :0x11:A :0x00:A:P
S:0x55:W:A :0x17:A :0xB0:A :0x01:AP
S:0x55:W:A :0x1A:A :0xA7:A :0x97:A :0xF4:A :0xC5:A :0x46:A :0x00:A:P
S:0x55:W:A :0x07:A :0x08:A:P
S:0x55:W:A :0x11:A :0x01:A:P
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
S:0x49:W:A :0x00:A S:0x49:R:A :MSB:A :LSB:N:P
```

tempCelsius = code * 0.0078125

---
## 47L64 EERAM w/ EEPROM

One 8kB bank:
- [0x0000] 64-bit magic number
- [0x0008] Configuration Data
- [0x0500] Temperature Compensation Data

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

---
## Integer Math

The `math.c` file contains functions for integer division and multiplication.  The multiplication routines utilize the MPY32 hardware multiplier and have been optimized for space.

Disassembled code for multiplication functions:

```
00008198 <mult16s16s>:
    8198:       82 4c c2 04     mov     r12,    &0x04c2 ;
    819c:       82 4d c8 04     mov     r13,    &0x04c8 ;
    81a0:       1c 42 ca 04     mov     &0x04ca,r12     ;0x04ca
    81a4:       1d 42 cc 04     mov     &0x04cc,r13     ;0x04cc
    81a8:       30 41           ret                     

000081aa <mult24s8s>:
    81aa:       82 4c d4 04     mov     r12,    &0x04d4 ;
    81ae:       82 4d d6 04     mov     r13,    &0x04d6 ;
    81b2:       82 4e c8 04     mov     r14,    &0x04c8 ;
    81b6:       1c 42 ca 04     mov     &0x04ca,r12     ;0x04ca
    81ba:       1d 42 cc 04     mov     &0x04cc,r13     ;0x04cc
    81be:       30 41           ret                     

000081c0 <mult32s32s>:
    81c0:       82 4c d4 04     mov     r12,    &0x04d4 ;
    81c4:       82 4d d6 04     mov     r13,    &0x04d6 ;
    81c8:       82 4e e0 04     mov     r14,    &0x04e0 ;
    81cc:       82 4f e2 04     mov     r15,    &0x04e2 ;
    81d0:       1c 42 e4 04     mov     &0x04e4,r12     ;0x04e4
    81d4:       1d 42 e6 04     mov     &0x04e6,r13     ;0x04e6
    81d8:       1e 42 e8 04     mov     &0x04e8,r14     ;0x04e8
    81dc:       1f 42 ea 04     mov     &0x04ea,r15     ;0x04ea
    81e0:       30 41           ret
```
