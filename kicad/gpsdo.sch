EESchema Schematic File Version 4
EELAYER 30 0
EELAYER END
$Descr B 17000 11000
encoding utf-8
Sheet 1 1
Title ""
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L MCU_Microchip_PIC32:PIC32MX795F512L-80x-PF U?
U 1 1 60FAE182
P 8350 5150
F 0 "U?" H 7750 2600 50  0000 C CNN
F 1 "PIC32MX795F512L-80x-PF" H 9100 2600 50  0000 C CNN
F 2 "Package_QFP:TQFP-100_14x14mm_P0.5mm" H 8350 2150 50  0001 C CNN
F 3 "http://ww1.microchip.com/downloads/en/DeviceDoc/60001156J.pdf" H 8350 2150 50  0001 C CNN
	1    8350 5150
	1    0    0    -1  
$EndComp
$Comp
L Oscillator:Si570 U1
U 1 1 60FCFECF
P 4050 4950
F 0 "U1" H 3700 5350 50  0000 C CNN
F 1 "549CAAC000111BBG" H 4500 4550 50  0000 C CNN
F 2 "Oscillator:Oscillator_SMD_SI570_SI571_Standard" H 4050 4650 50  0001 C CNN
F 3 "http://www.silabs.com/Support%20Documents/TechnicalDocs/si570.pdf" H 3650 5650 50  0001 C CNN
	1    4050 4950
	1    0    0    -1  
$EndComp
$Comp
L RF_GPS:MAX-8Q U2
U 1 1 60FD23E0
P 4050 3000
F 0 "U2" H 4250 2350 50  0000 C CNN
F 1 "MAX-8Q" H 4350 2250 50  0000 C CNN
F 2 "RF_GPS:ublox_MAX" H 4450 2350 50  0001 C CNN
F 3 "https://www.u-blox.com/sites/default/files/MAX-8_DataSheet_%28UBX-16000093%29.pdf" H 4050 3000 50  0001 C CNN
	1    4050 3000
	1    0    0    -1  
$EndComp
$Comp
L Sensor_Temperature:Si7051-A20 U3
U 1 1 60FD391B
P 4050 6050
F 0 "U3" H 4394 6096 50  0000 L CNN
F 1 "Si7051-A20" H 4394 6005 50  0000 L CNN
F 2 "Package_DFN_QFN:DFN-6-1EP_3x3mm_P1mm_EP1.65x2.55mm" H 4050 5650 50  0001 C CNN
F 3 "https://www.silabs.com/documents/public/data-sheets/Si7050-1-3-4-5-A20.pdf" H 3850 6350 50  0001 C CNN
	1    4050 6050
	1    0    0    -1  
$EndComp
$Comp
L Connector:RJ45_Abracon_ARJP11A-MASA-B-A-EMU2 J?
U 1 1 60FD505B
P 15250 2450
F 0 "J?" H 15250 3967 50  0000 C CNN
F 1 "RJ45_Abracon_ARJP11A-MASA-B-A-EMU2" H 15250 3876 50  0000 C CNN
F 2 "Connector_RJ:RJ45_Abracon_ARJP11A-MA_Horizontal" H 15250 3850 50  0001 C CNN
F 3 "https://abracon.com/Magnetics/lan/ARJP11A.PDF" H 15100 1600 50  0001 C CNN
	1    15250 2450
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR?
U 1 1 60FDC4EE
P 14550 2150
F 0 "#PWR?" H 14550 1900 50  0001 C CNN
F 1 "GND" V 14555 2022 50  0000 R CNN
F 2 "" H 14550 2150 50  0001 C CNN
F 3 "" H 14550 2150 50  0001 C CNN
	1    14550 2150
	0    1    1    0   
$EndComp
$Comp
L power:GND #PWR?
U 1 1 60FDFB2F
P 4050 6350
F 0 "#PWR?" H 4050 6100 50  0001 C CNN
F 1 "GND" H 4055 6177 50  0000 C CNN
F 2 "" H 4050 6350 50  0001 C CNN
F 3 "" H 4050 6350 50  0001 C CNN
	1    4050 6350
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR?
U 1 1 60FDFF77
P 4050 5350
F 0 "#PWR?" H 4050 5100 50  0001 C CNN
F 1 "GND" H 4055 5177 50  0000 C CNN
F 2 "" H 4050 5350 50  0001 C CNN
F 3 "" H 4050 5350 50  0001 C CNN
	1    4050 5350
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR?
U 1 1 60FE03FF
P 4050 3700
F 0 "#PWR?" H 4050 3450 50  0001 C CNN
F 1 "GND" H 4055 3527 50  0000 C CNN
F 2 "" H 4050 3700 50  0001 C CNN
F 3 "" H 4050 3700 50  0001 C CNN
	1    4050 3700
	1    0    0    -1  
$EndComp
Wire Wire Line
	4550 4850 5600 4850
Wire Wire Line
	5600 4850 5600 4550
Wire Wire Line
	5600 4550 7150 4550
$Comp
L power:GND #PWR?
U 1 1 60FE526E
P 8250 7750
F 0 "#PWR?" H 8250 7500 50  0001 C CNN
F 1 "GND" H 8255 7577 50  0000 C CNN
F 2 "" H 8250 7750 50  0001 C CNN
F 3 "" H 8250 7750 50  0001 C CNN
	1    8250 7750
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR?
U 1 1 60FE55A2
P 8450 7750
F 0 "#PWR?" H 8450 7500 50  0001 C CNN
F 1 "GND" H 8455 7577 50  0000 C CNN
F 2 "" H 8450 7750 50  0001 C CNN
F 3 "" H 8450 7750 50  0001 C CNN
	1    8450 7750
	1    0    0    -1  
$EndComp
$EndSCHEMATC
