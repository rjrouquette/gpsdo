EESchema Schematic File Version 4
EELAYER 30 0
EELAYER END
$Descr A 11000 8500
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
L Oscillator:Si570 U3
U 1 1 60FCFECF
P 4650 6050
F 0 "U3" H 4300 6450 50  0000 C CNN
F 1 "549CAAC000111BBG" H 5100 5650 50  0000 C CNN
F 2 "Oscillator:Oscillator_SMD_SI570_SI571_Standard" H 4650 5750 50  0001 C CNN
F 3 "http://www.silabs.com/Support%20Documents/TechnicalDocs/si570.pdf" H 4250 6750 50  0001 C CNN
	1    4650 6050
	1    0    0    -1  
$EndComp
$Comp
L RF_GPS:MAX-8Q U2
U 1 1 60FD23E0
P 2300 6950
F 0 "U2" H 2500 6300 50  0000 C CNN
F 1 "MAX-8Q" H 2600 6200 50  0000 C CNN
F 2 "RF_GPS:ublox_MAX" H 2700 6300 50  0001 C CNN
F 3 "https://www.u-blox.com/sites/default/files/MAX-8_DataSheet_%28UBX-16000093%29.pdf" H 2300 6950 50  0001 C CNN
	1    2300 6950
	-1   0    0    -1  
$EndComp
$Comp
L Sensor_Temperature:Si7051-A20 U4
U 1 1 60FD391B
P 4650 7250
F 0 "U4" H 4994 7296 50  0000 L CNN
F 1 "Si7051-A20" H 4994 7205 50  0000 L CNN
F 2 "Package_DFN_QFN:DFN-6-1EP_3x3mm_P1mm_EP1.5x2.4mm" H 4650 6850 50  0001 C CNN
F 3 "https://www.silabs.com/documents/public/data-sheets/Si7050-1-3-4-5-A20.pdf" H 4450 7550 50  0001 C CNN
	1    4650 7250
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR012
U 1 1 60FDFB2F
P 4650 7550
F 0 "#PWR012" H 4650 7300 50  0001 C CNN
F 1 "GND" H 4655 7377 50  0000 C CNN
F 2 "" H 4650 7550 50  0001 C CNN
F 3 "" H 4650 7550 50  0001 C CNN
	1    4650 7550
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR010
U 1 1 60FDFF77
P 4650 6450
F 0 "#PWR010" H 4650 6200 50  0001 C CNN
F 1 "GND" H 4655 6277 50  0000 C CNN
F 2 "" H 4650 6450 50  0001 C CNN
F 3 "" H 4650 6450 50  0001 C CNN
	1    4650 6450
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR07
U 1 1 60FE03FF
P 2300 7650
F 0 "#PWR07" H 2300 7400 50  0001 C CNN
F 1 "GND" H 2305 7477 50  0000 C CNN
F 2 "" H 2300 7650 50  0001 C CNN
F 3 "" H 2300 7650 50  0001 C CNN
	1    2300 7650
	-1   0    0    -1  
$EndComp
Wire Wire Line
	4150 6150 3900 6150
Wire Wire Line
	3900 7250 4250 7250
Wire Wire Line
	4250 7150 4000 7150
Wire Wire Line
	4000 6250 4150 6250
$Comp
L power:+3.3V #PWR011
U 1 1 60FB36E1
P 4650 6950
F 0 "#PWR011" H 4650 6800 50  0001 C CNN
F 1 "+3.3V" H 4665 7123 50  0000 C CNN
F 2 "" H 4650 6950 50  0001 C CNN
F 3 "" H 4650 6950 50  0001 C CNN
	1    4650 6950
	1    0    0    -1  
$EndComp
$Comp
L power:+3.3V #PWR09
U 1 1 60FB3C13
P 4650 5650
F 0 "#PWR09" H 4650 5500 50  0001 C CNN
F 1 "+3.3V" H 4665 5823 50  0000 C CNN
F 2 "" H 4650 5650 50  0001 C CNN
F 3 "" H 4650 5650 50  0001 C CNN
	1    4650 5650
	1    0    0    -1  
$EndComp
Wire Notes Line
	3750 5350 6150 5350
Wire Notes Line
	6150 7850 3750 7850
$Comp
L Connector:Conn_Coaxial J1
U 1 1 60FBD0FE
P 1200 6950
F 0 "J1" H 1300 6925 50  0000 L CNN
F 1 "Conn_Coaxial" H 1300 6834 50  0000 L CNN
F 2 "Connector_Coaxial:SMA_Molex_73251-1153_EdgeMount_Horizontal" H 1200 6950 50  0001 C CNN
F 3 " ~" H 1200 6950 50  0001 C CNN
	1    1200 6950
	-1   0    0    -1  
$EndComp
$Comp
L power:GND #PWR01
U 1 1 60FBDB7E
P 1200 7150
F 0 "#PWR01" H 1200 6900 50  0001 C CNN
F 1 "GND" H 1205 6977 50  0000 C CNN
F 2 "" H 1200 7150 50  0001 C CNN
F 3 "" H 1200 7150 50  0001 C CNN
	1    1200 7150
	-1   0    0    -1  
$EndComp
$Comp
L Device:L_Small L1
U 1 1 60FBF33B
P 1500 6700
F 0 "L1" H 1548 6746 50  0000 L CNN
F 1 "27n" H 1548 6655 50  0000 L CNN
F 2 "Resistor_SMD:R_0603_1608Metric" H 1500 6700 50  0001 C CNN
F 3 "~" H 1500 6700 50  0001 C CNN
	1    1500 6700
	-1   0    0    -1  
$EndComp
Wire Wire Line
	1500 6800 1500 6950
Connection ~ 1500 6950
Wire Wire Line
	1500 6950 1400 6950
$Comp
L Device:C_Small C1
U 1 1 60FC1266
P 1350 6500
F 0 "C1" V 1121 6500 50  0000 C CNN
F 1 "10n" V 1212 6500 50  0000 C CNN
F 2 "Resistor_SMD:R_0603_1608Metric" H 1350 6500 50  0001 C CNN
F 3 "~" H 1350 6500 50  0001 C CNN
	1    1350 6500
	0    -1   1    0   
$EndComp
Connection ~ 1500 6500
Wire Wire Line
	1500 6500 1500 6600
Wire Wire Line
	1450 6500 1500 6500
$Comp
L power:GND #PWR02
U 1 1 60FC3F9C
P 1250 6500
F 0 "#PWR02" H 1250 6250 50  0001 C CNN
F 1 "GND" V 1255 6372 50  0000 R CNN
F 2 "" H 1250 6500 50  0001 C CNN
F 3 "" H 1250 6500 50  0001 C CNN
	1    1250 6500
	0    1    -1   0   
$EndComp
$Comp
L power:+3.3V #PWR08
U 1 1 60FD182D
P 2400 6150
F 0 "#PWR08" H 2400 6000 50  0001 C CNN
F 1 "+3.3V" H 2415 6323 50  0000 C CNN
F 2 "" H 2400 6150 50  0001 C CNN
F 3 "" H 2400 6150 50  0001 C CNN
	1    2400 6150
	-1   0    0    -1  
$EndComp
Wire Wire Line
	2400 6150 2400 6200
Wire Wire Line
	2300 6250 2300 6200
Wire Wire Line
	2300 6200 2400 6200
Connection ~ 2400 6200
Wire Wire Line
	2400 6200 2400 6250
Wire Wire Line
	2500 6250 2500 6200
Wire Wire Line
	2500 6200 2400 6200
Wire Wire Line
	2100 6250 2100 6200
Wire Wire Line
	1500 6200 1500 6500
Wire Wire Line
	1700 6950 1500 6950
$Comp
L Device:R_Small_US R1
U 1 1 60FDF5E7
P 1700 6200
F 0 "R1" V 1495 6200 50  0000 C CNN
F 1 "10" V 1586 6200 50  0000 C CNN
F 2 "Resistor_SMD:R_0603_1608Metric" H 1700 6200 50  0001 C CNN
F 3 "~" H 1700 6200 50  0001 C CNN
	1    1700 6200
	0    -1   1    0   
$EndComp
Wire Wire Line
	1600 6200 1500 6200
Wire Wire Line
	1800 6200 2100 6200
$Comp
L Device:C_Small C3
U 1 1 60FE6639
P 5850 6550
F 0 "C3" V 5621 6550 50  0000 C CNN
F 1 "100n" V 5712 6550 50  0000 C CNN
F 2 "Resistor_SMD:R_0603_1608Metric" H 5850 6550 50  0001 C CNN
F 3 "~" H 5850 6550 50  0001 C CNN
	1    5850 6550
	1    0    0    -1  
$EndComp
$Comp
L Device:C_Small C4
U 1 1 60FE9AFC
P 5850 7250
F 0 "C4" V 5621 7250 50  0000 C CNN
F 1 "100n" V 5712 7250 50  0000 C CNN
F 2 "Resistor_SMD:R_0603_1608Metric" H 5850 7250 50  0001 C CNN
F 3 "~" H 5850 7250 50  0001 C CNN
	1    5850 7250
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR016
U 1 1 60FEF08B
P 5850 7350
F 0 "#PWR016" H 5850 7100 50  0001 C CNN
F 1 "GND" H 5855 7177 50  0000 C CNN
F 2 "" H 5850 7350 50  0001 C CNN
F 3 "" H 5850 7350 50  0001 C CNN
	1    5850 7350
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR014
U 1 1 60FEF433
P 5850 6650
F 0 "#PWR014" H 5850 6400 50  0001 C CNN
F 1 "GND" H 5855 6477 50  0000 C CNN
F 2 "" H 5850 6650 50  0001 C CNN
F 3 "" H 5850 6650 50  0001 C CNN
	1    5850 6650
	1    0    0    -1  
$EndComp
$Comp
L power:+3.3V #PWR015
U 1 1 60FF22E7
P 5850 7150
F 0 "#PWR015" H 5850 7000 50  0001 C CNN
F 1 "+3.3V" H 5865 7323 50  0000 C CNN
F 2 "" H 5850 7150 50  0001 C CNN
F 3 "" H 5850 7150 50  0001 C CNN
	1    5850 7150
	1    0    0    -1  
$EndComp
$Comp
L power:+3.3V #PWR013
U 1 1 60FF2603
P 5850 6450
F 0 "#PWR013" H 5850 6300 50  0001 C CNN
F 1 "+3.3V" H 5865 6623 50  0000 C CNN
F 2 "" H 5850 6450 50  0001 C CNN
F 3 "" H 5850 6450 50  0001 C CNN
	1    5850 6450
	1    0    0    -1  
$EndComp
Wire Wire Line
	5150 5950 5400 5950
Text GLabel 5400 5950 2    50   Input ~ 0
CLK
Wire Notes Line
	6150 5350 6150 7850
Wire Notes Line
	3750 5350 3750 7850
$Comp
L gpsdo-rescue:MSP430F5171IDA-2021-07-26_01-49-48 U1
U 1 1 6103888A
P 1600 1200
F 0 "U1" H 5500 1587 60  0000 C CNN
F 1 "MSP430F5171IDA" H 5500 1481 60  0000 C CNN
F 2 "Package_SO:TSSOP-38_4.4x9.7mm_P0.5mm" H 5500 1440 60  0001 C CNN
F 3 "" H 1600 1200 60  0000 C CNN
	1    1600 1200
	1    0    0    -1  
$EndComp
$Comp
L power:+3.3V #PWR03
U 1 1 610729CD
P 1600 1200
F 0 "#PWR03" H 1600 1050 50  0001 C CNN
F 1 "+3.3V" V 1600 1450 50  0000 C CNN
F 2 "" H 1600 1200 50  0001 C CNN
F 3 "" H 1600 1200 50  0001 C CNN
	1    1600 1200
	0    -1   -1   0   
$EndComp
$Comp
L power:+3.3V #PWR021
U 1 1 61073AE8
P 9400 1900
F 0 "#PWR021" H 9400 1750 50  0001 C CNN
F 1 "+3.3V" V 9400 2150 50  0000 C CNN
F 2 "" H 9400 1900 50  0001 C CNN
F 3 "" H 9400 1900 50  0001 C CNN
	1    9400 1900
	0    1    1    0   
$EndComp
$Comp
L power:+3.3V #PWR024
U 1 1 61075175
P 9400 2900
F 0 "#PWR024" H 9400 2750 50  0001 C CNN
F 1 "+3.3V" V 9400 3150 50  0000 C CNN
F 2 "" H 9400 2900 50  0001 C CNN
F 3 "" H 9400 2900 50  0001 C CNN
	1    9400 2900
	0    1    1    0   
$EndComp
$Comp
L Device:C_Small C7
U 1 1 61076C6C
P 9600 2100
F 0 "C7" V 9500 1950 50  0000 C CNN
F 1 "470n" V 9462 2100 50  0000 C CNN
F 2 "Capacitor_SMD:C_0805_2012Metric" H 9600 2100 50  0001 C CNN
F 3 "~" H 9600 2100 50  0001 C CNN
	1    9600 2100
	0    -1   -1   0   
$EndComp
Wire Wire Line
	9500 2100 9400 2100
$Comp
L Device:C_Small C5
U 1 1 6107E676
P 8000 700
F 0 "C5" V 7771 700 50  0000 C CNN
F 1 "100n" V 7862 700 50  0000 C CNN
F 2 "Resistor_SMD:R_0603_1608Metric" H 8000 700 50  0001 C CNN
F 3 "~" H 8000 700 50  0001 C CNN
	1    8000 700 
	0    -1   -1   0   
$EndComp
$Comp
L power:GND #PWR018
U 1 1 6107E67C
P 8100 700
F 0 "#PWR018" H 8100 450 50  0001 C CNN
F 1 "GND" H 8105 527 50  0000 C CNN
F 2 "" H 8100 700 50  0001 C CNN
F 3 "" H 8100 700 50  0001 C CNN
	1    8100 700 
	0    -1   -1   0   
$EndComp
$Comp
L power:+3.3V #PWR017
U 1 1 6107E682
P 7900 700
F 0 "#PWR017" H 7900 550 50  0001 C CNN
F 1 "+3.3V" H 7915 873 50  0000 C CNN
F 2 "" H 7900 700 50  0001 C CNN
F 3 "" H 7900 700 50  0001 C CNN
	1    7900 700 
	0    -1   -1   0   
$EndComp
$Comp
L Device:C_Small C6
U 1 1 6107F05D
P 8750 700
F 0 "C6" V 8521 700 50  0000 C CNN
F 1 "100n" V 8612 700 50  0000 C CNN
F 2 "Resistor_SMD:R_0603_1608Metric" H 8750 700 50  0001 C CNN
F 3 "~" H 8750 700 50  0001 C CNN
	1    8750 700 
	0    -1   -1   0   
$EndComp
$Comp
L power:GND #PWR020
U 1 1 6107F063
P 8850 700
F 0 "#PWR020" H 8850 450 50  0001 C CNN
F 1 "GND" H 8855 527 50  0000 C CNN
F 2 "" H 8850 700 50  0001 C CNN
F 3 "" H 8850 700 50  0001 C CNN
	1    8850 700 
	0    -1   -1   0   
$EndComp
$Comp
L power:+3.3V #PWR019
U 1 1 6107F069
P 8650 700
F 0 "#PWR019" H 8650 550 50  0001 C CNN
F 1 "+3.3V" H 8665 873 50  0000 C CNN
F 2 "" H 8650 700 50  0001 C CNN
F 3 "" H 8650 700 50  0001 C CNN
	1    8650 700 
	0    -1   -1   0   
$EndComp
$Comp
L Device:C_Small C2
U 1 1 6107FBF8
P 2200 700
F 0 "C2" V 1971 700 50  0000 C CNN
F 1 "100n" V 2062 700 50  0000 C CNN
F 2 "Resistor_SMD:R_0603_1608Metric" H 2200 700 50  0001 C CNN
F 3 "~" H 2200 700 50  0001 C CNN
	1    2200 700 
	0    -1   -1   0   
$EndComp
$Comp
L power:GND #PWR06
U 1 1 6107FBFE
P 2300 700
F 0 "#PWR06" H 2300 450 50  0001 C CNN
F 1 "GND" H 2305 527 50  0000 C CNN
F 2 "" H 2300 700 50  0001 C CNN
F 3 "" H 2300 700 50  0001 C CNN
	1    2300 700 
	0    -1   -1   0   
$EndComp
$Comp
L power:+3.3V #PWR05
U 1 1 6107FC04
P 2100 700
F 0 "#PWR05" H 2100 550 50  0001 C CNN
F 1 "+3.3V" H 2115 873 50  0000 C CNN
F 2 "" H 2100 700 50  0001 C CNN
F 3 "" H 2100 700 50  0001 C CNN
	1    2100 700 
	0    -1   -1   0   
$EndComp
$Comp
L power:GND #PWR025
U 1 1 61080FF2
P 9700 2100
F 0 "#PWR025" H 9700 1850 50  0001 C CNN
F 1 "GND" V 9700 1900 50  0000 C CNN
F 2 "" H 9700 2100 50  0001 C CNN
F 3 "" H 9700 2100 50  0001 C CNN
	1    9700 2100
	0    -1   -1   0   
$EndComp
$Comp
L power:GND #PWR022
U 1 1 61082270
P 9400 2000
F 0 "#PWR022" H 9400 1750 50  0001 C CNN
F 1 "GND" V 9400 1800 50  0000 C CNN
F 2 "" H 9400 2000 50  0001 C CNN
F 3 "" H 9400 2000 50  0001 C CNN
	1    9400 2000
	0    -1   -1   0   
$EndComp
$Comp
L power:GND #PWR023
U 1 1 6108251E
P 9400 2800
F 0 "#PWR023" H 9400 2550 50  0001 C CNN
F 1 "GND" V 9400 2600 50  0000 C CNN
F 2 "" H 9400 2800 50  0001 C CNN
F 3 "" H 9400 2800 50  0001 C CNN
	1    9400 2800
	0    -1   -1   0   
$EndComp
$Comp
L power:GND #PWR04
U 1 1 610844C4
P 1600 1500
F 0 "#PWR04" H 1600 1250 50  0001 C CNN
F 1 "GND" V 1600 1300 50  0000 C CNN
F 2 "" H 1600 1500 50  0001 C CNN
F 3 "" H 1600 1500 50  0001 C CNN
	1    1600 1500
	0    1    1    0   
$EndComp
Text GLabel 1600 1400 0    50   Input ~ 0
CLK
Text GLabel 1700 7250 0    50   Input ~ 0
gPPS
Text GLabel 1600 3000 0    50   Input ~ 0
gPPS
$Comp
L Connector:Conn_01x04_Male J2
U 1 1 61001312
P 9650 4550
F 0 "J2" H 9758 4831 50  0000 C CNN
F 1 "Conn_01x04_Male" H 9758 4740 50  0000 C CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x04_P2.54mm_Vertical" H 9650 4550 50  0001 C CNN
F 3 "~" H 9650 4550 50  0001 C CNN
	1    9650 4550
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR0101
U 1 1 61004A66
P 9850 4450
F 0 "#PWR0101" H 9850 4200 50  0001 C CNN
F 1 "GND" V 9850 4250 50  0000 C CNN
F 2 "" H 9850 4450 50  0001 C CNN
F 3 "" H 9850 4450 50  0001 C CNN
	1    9850 4450
	0    -1   -1   0   
$EndComp
$Comp
L power:GND #PWR0102
U 1 1 61004F29
P 9850 4550
F 0 "#PWR0102" H 9850 4300 50  0001 C CNN
F 1 "GND" V 9850 4350 50  0000 C CNN
F 2 "" H 9850 4550 50  0001 C CNN
F 3 "" H 9850 4550 50  0001 C CNN
	1    9850 4550
	0    -1   -1   0   
$EndComp
$Comp
L Connector:Conn_01x04_Male J3
U 1 1 61012471
P 9650 5150
F 0 "J3" H 9758 5431 50  0000 C CNN
F 1 "Conn_01x04_Male" H 9758 5340 50  0000 C CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x04_P2.54mm_Vertical" H 9650 5150 50  0001 C CNN
F 3 "~" H 9650 5150 50  0001 C CNN
	1    9650 5150
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR0105
U 1 1 61012477
P 9850 5050
F 0 "#PWR0105" H 9850 4800 50  0001 C CNN
F 1 "GND" V 9850 4850 50  0000 C CNN
F 2 "" H 9850 5050 50  0001 C CNN
F 3 "" H 9850 5050 50  0001 C CNN
	1    9850 5050
	0    -1   -1   0   
$EndComp
$Comp
L power:GND #PWR0106
U 1 1 6101247D
P 9850 5150
F 0 "#PWR0106" H 9850 4900 50  0001 C CNN
F 1 "GND" V 9850 4950 50  0000 C CNN
F 2 "" H 9850 5150 50  0001 C CNN
F 3 "" H 9850 5150 50  0001 C CNN
	1    9850 5150
	0    -1   -1   0   
$EndComp
$Comp
L Connector:Conn_01x04_Male J4
U 1 1 61013E1B
P 700 1800
F 0 "J4" H 808 2081 50  0000 C CNN
F 1 "Conn_01x04_Male" H 808 1990 50  0000 C CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x04_P2.54mm_Vertical" H 700 1800 50  0001 C CNN
F 3 "~" H 700 1800 50  0001 C CNN
	1    700  1800
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR0111
U 1 1 61013E2D
P 900 1900
F 0 "#PWR0111" H 900 1650 50  0001 C CNN
F 1 "GND" V 900 1700 50  0000 C CNN
F 2 "" H 900 1900 50  0001 C CNN
F 3 "" H 900 1900 50  0001 C CNN
	1    900  1900
	0    -1   -1   0   
$EndComp
$Comp
L power:GND #PWR0112
U 1 1 61013E33
P 900 2000
F 0 "#PWR0112" H 900 1750 50  0001 C CNN
F 1 "GND" V 900 1800 50  0000 C CNN
F 2 "" H 900 2000 50  0001 C CNN
F 3 "" H 900 2000 50  0001 C CNN
	1    900  2000
	0    -1   -1   0   
$EndComp
$Comp
L Connector:Conn_01x04_Male J5
U 1 1 61014761
P 700 5900
F 0 "J5" H 808 6181 50  0000 C CNN
F 1 "Conn_01x04_Male" H 808 6090 50  0000 C CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x04_P2.54mm_Vertical" H 700 5900 50  0001 C CNN
F 3 "~" H 700 5900 50  0001 C CNN
	1    700  5900
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR0115
U 1 1 61014773
P 900 6000
F 0 "#PWR0115" H 900 5750 50  0001 C CNN
F 1 "GND" V 900 5800 50  0000 C CNN
F 2 "" H 900 6000 50  0001 C CNN
F 3 "" H 900 6000 50  0001 C CNN
	1    900  6000
	0    -1   -1   0   
$EndComp
$Comp
L power:GND #PWR0116
U 1 1 61014779
P 900 6100
F 0 "#PWR0116" H 900 5850 50  0001 C CNN
F 1 "GND" V 900 5900 50  0000 C CNN
F 2 "" H 900 6100 50  0001 C CNN
F 3 "" H 900 6100 50  0001 C CNN
	1    900  6100
	0    -1   -1   0   
$EndComp
Text GLabel 1600 2900 0    50   Input ~ 0
PPS
Wire Wire Line
	4000 6250 4000 7150
Wire Wire Line
	3900 6150 3900 7250
Wire Wire Line
	1200 2000 1600 2000
Wire Wire Line
	1600 2100 1300 2100
Wire Wire Line
	1300 2100 1300 4100
Connection ~ 3900 6150
Connection ~ 4000 6250
Wire Wire Line
	2900 6950 3200 6950
Wire Wire Line
	3200 6950 3200 4950
Wire Wire Line
	3100 6850 2900 6850
$Comp
L Device:R_Small_US R2
U 1 1 6101A651
P 1000 4100
F 0 "R2" V 795 4100 50  0000 C CNN
F 1 "1.5k" V 886 4100 50  0000 C CNN
F 2 "Resistor_SMD:R_0603_1608Metric" H 1000 4100 50  0001 C CNN
F 3 "~" H 1000 4100 50  0001 C CNN
	1    1000 4100
	0    -1   1    0   
$EndComp
$Comp
L Device:R_Small_US R3
U 1 1 6101AE67
P 1000 4200
F 0 "R3" V 795 4200 50  0000 C CNN
F 1 "1.5k" V 886 4200 50  0000 C CNN
F 2 "Resistor_SMD:R_0603_1608Metric" H 1000 4200 50  0001 C CNN
F 3 "~" H 1000 4200 50  0001 C CNN
	1    1000 4200
	0    1    -1   0   
$EndComp
$Comp
L power:+3.3V #PWR0124
U 1 1 6101B863
P 900 4100
F 0 "#PWR0124" H 900 3950 50  0001 C CNN
F 1 "+3.3V" V 900 4350 50  0000 C CNN
F 2 "" H 900 4100 50  0001 C CNN
F 3 "" H 900 4100 50  0001 C CNN
	1    900  4100
	0    -1   -1   0   
$EndComp
$Comp
L power:+3.3V #PWR0125
U 1 1 6101BDD8
P 900 4200
F 0 "#PWR0125" H 900 4050 50  0001 C CNN
F 1 "+3.3V" V 900 4450 50  0000 C CNN
F 2 "" H 900 4200 50  0001 C CNN
F 3 "" H 900 4200 50  0001 C CNN
	1    900  4200
	0    -1   -1   0   
$EndComp
Wire Wire Line
	1100 4100 1300 4100
Connection ~ 1300 4100
Wire Wire Line
	1100 4200 1200 4200
Connection ~ 1200 4200
Wire Wire Line
	1200 4200 1200 2000
Wire Wire Line
	2900 6650 3000 6650
Wire Wire Line
	3000 6650 3000 5800
Wire Wire Line
	2900 6550 2900 5900
Wire Wire Line
	2900 5900 900  5900
Wire Wire Line
	3000 5800 900  5800
Wire Wire Line
	900  1700 1600 1700
Wire Wire Line
	900  1800 1600 1800
Wire Wire Line
	4950 7350 5100 7350
Wire Wire Line
	5100 7350 5100 6950
Wire Wire Line
	5100 6950 4650 6950
Connection ~ 4650 6950
Wire Wire Line
	4250 5850 4150 5850
Wire Wire Line
	4150 5850 4150 5650
Wire Wire Line
	4150 5650 4650 5650
Connection ~ 4650 5650
$Comp
L power:+3.3V #PWR0103
U 1 1 611AB77B
P 9850 4650
F 0 "#PWR0103" H 9850 4500 50  0001 C CNN
F 1 "+3.3V" V 9850 4900 50  0000 C CNN
F 2 "" H 9850 4650 50  0001 C CNN
F 3 "" H 9850 4650 50  0001 C CNN
	1    9850 4650
	0    1    -1   0   
$EndComp
$Comp
L Device:C_Small C10
U 1 1 611B610F
P 8800 4550
F 0 "C10" V 8571 4550 50  0000 C CNN
F 1 "22u" V 8662 4550 50  0000 C CNN
F 2 "Capacitor_SMD:C_0805_2012Metric" H 8800 4550 50  0001 C CNN
F 3 "~" H 8800 4550 50  0001 C CNN
	1    8800 4550
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR0110
U 1 1 611B6A7D
P 8800 4650
F 0 "#PWR0110" H 8800 4400 50  0001 C CNN
F 1 "GND" H 8805 4477 50  0000 C CNN
F 2 "" H 8800 4650 50  0001 C CNN
F 3 "" H 8800 4650 50  0001 C CNN
	1    8800 4650
	1    0    0    -1  
$EndComp
$Comp
L power:+3.3V #PWR0113
U 1 1 611B6E53
P 8800 4450
F 0 "#PWR0113" H 8800 4300 50  0001 C CNN
F 1 "+3.3V" H 8815 4623 50  0000 C CNN
F 2 "" H 8800 4450 50  0001 C CNN
F 3 "" H 8800 4450 50  0001 C CNN
	1    8800 4450
	1    0    0    -1  
$EndComp
Wire Wire Line
	2900 7350 3300 7350
Wire Wire Line
	3300 7350 3300 5700
Wire Wire Line
	3300 5700 1400 5700
$Comp
L Device:C_Small C11
U 1 1 611D4830
P 850 7550
F 0 "C11" V 621 7550 50  0000 C CNN
F 1 "100n" V 712 7550 50  0000 C CNN
F 2 "Resistor_SMD:R_0603_1608Metric" H 850 7550 50  0001 C CNN
F 3 "~" H 850 7550 50  0001 C CNN
	1    850  7550
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR0107
U 1 1 611D4836
P 850 7650
F 0 "#PWR0107" H 850 7400 50  0001 C CNN
F 1 "GND" H 855 7477 50  0000 C CNN
F 2 "" H 850 7650 50  0001 C CNN
F 3 "" H 850 7650 50  0001 C CNN
	1    850  7650
	1    0    0    -1  
$EndComp
$Comp
L power:+3.3V #PWR0108
U 1 1 611D483C
P 850 7450
F 0 "#PWR0108" H 850 7300 50  0001 C CNN
F 1 "+3.3V" H 865 7623 50  0000 C CNN
F 2 "" H 850 7450 50  0001 C CNN
F 3 "" H 850 7450 50  0001 C CNN
	1    850  7450
	1    0    0    -1  
$EndComp
Text GLabel 1600 2700 0    50   Input ~ 0
PPS
$Comp
L power:+3.3V #PWR0151
U 1 1 61125D59
P 9850 4750
F 0 "#PWR0151" H 9850 4600 50  0001 C CNN
F 1 "+3.3V" V 9850 5000 50  0000 C CNN
F 2 "" H 9850 4750 50  0001 C CNN
F 3 "" H 9850 4750 50  0001 C CNN
	1    9850 4750
	0    1    -1   0   
$EndComp
Text GLabel 1600 2600 0    50   Input ~ 0
gPPS
$Comp
L Connector:Conn_01x04_Male J6
U 1 1 611530AF
P 10350 950
F 0 "J6" H 10458 1231 50  0000 C CNN
F 1 "Conn_01x04_Male" H 10458 1140 50  0000 C CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x04_P2.54mm_Vertical" H 10350 950 50  0001 C CNN
F 3 "~" H 10350 950 50  0001 C CNN
	1    10350 950 
	-1   0    0    1   
$EndComp
$Comp
L power:GND #PWR0152
U 1 1 61153DA5
P 10150 750
F 0 "#PWR0152" H 10150 500 50  0001 C CNN
F 1 "GND" V 10050 700 50  0000 C CNN
F 2 "" H 10150 750 50  0001 C CNN
F 3 "" H 10150 750 50  0001 C CNN
	1    10150 750 
	0    1    1    0   
$EndComp
Wire Wire Line
	9400 1500 9800 1500
Wire Wire Line
	9800 1500 9800 950 
Wire Wire Line
	9800 950  10150 950 
Wire Wire Line
	9400 1400 9700 1400
Wire Wire Line
	9700 1400 9700 850 
Wire Wire Line
	9700 850  10150 850 
$Comp
L power:+3.3V #PWR0153
U 1 1 6115BED8
P 10150 1050
F 0 "#PWR0153" H 10150 900 50  0001 C CNN
F 1 "+3.3V" V 10150 1300 50  0000 C CNN
F 2 "" H 10150 1050 50  0001 C CNN
F 3 "" H 10150 1050 50  0001 C CNN
	1    10150 1050
	0    -1   1    0   
$EndComp
Text Label 9700 1250 1    50   ~ 0
SBWTDIO
Text Label 9800 1250 1    50   ~ 0
SBWTCK
Text Label 1200 3500 1    50   ~ 0
SDA
Text Label 1300 3500 1    50   ~ 0
SCL
Text Label 1400 5900 0    50   ~ 0
gRXD
Text Label 1400 5800 0    50   ~ 0
gTXD
Text Label 1000 1700 0    50   ~ 0
uTXD
Text Label 1000 1800 0    50   ~ 0
uRXD
$Comp
L Device:R_Small_US R10
U 1 1 6101D1B0
P 9550 850
F 0 "R10" V 9345 850 50  0000 C CNN
F 1 "47k" V 9436 850 50  0000 C CNN
F 2 "Resistor_SMD:R_0603_1608Metric" H 9550 850 50  0001 C CNN
F 3 "~" H 9550 850 50  0001 C CNN
	1    9550 850 
	0    -1   1    0   
$EndComp
Wire Wire Line
	9650 850  9700 850 
Connection ~ 9700 850 
$Comp
L power:+3.3V #PWR0154
U 1 1 6102115A
P 9450 850
F 0 "#PWR0154" H 9450 700 50  0001 C CNN
F 1 "+3.3V" V 9450 1100 50  0000 C CNN
F 2 "" H 9450 850 50  0001 C CNN
F 3 "" H 9450 850 50  0001 C CNN
	1    9450 850 
	0    -1   1    0   
$EndComp
$Comp
L Memory_NVRAM:47L16 U11
U 1 1 6101E0D3
P 5100 4100
F 0 "U11" H 5300 3700 50  0000 C CNN
F 1 "47L64" H 5300 3600 50  0000 C CNN
F 2 "Package_SO:SOIC-8_3.9x4.9mm_P1.27mm" H 5100 3650 50  0001 C CNN
F 3 "http://ww1.microchip.com/downloads/en/DeviceDoc/20005371C.pdf" H 5350 4450 50  0001 C CNN
	1    5100 4100
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR0155
U 1 1 61022F8C
P 5100 4500
F 0 "#PWR0155" H 5100 4250 50  0001 C CNN
F 1 "GND" H 5105 4327 50  0000 C CNN
F 2 "" H 5100 4500 50  0001 C CNN
F 3 "" H 5100 4500 50  0001 C CNN
	1    5100 4500
	1    0    0    -1  
$EndComp
$Comp
L power:+3.3V #PWR0156
U 1 1 61023732
P 5100 3800
F 0 "#PWR0156" H 5100 3650 50  0001 C CNN
F 1 "+3.3V" H 5115 3973 50  0000 C CNN
F 2 "" H 5100 3800 50  0001 C CNN
F 3 "" H 5100 3800 50  0001 C CNN
	1    5100 3800
	1    0    0    -1  
$EndComp
Wire Wire Line
	4700 4100 3900 4100
Wire Wire Line
	4700 4000 4000 4000
Connection ~ 4000 4950
Wire Wire Line
	4000 4950 4000 6250
Connection ~ 3900 5050
Wire Wire Line
	3900 5050 3900 6150
$Comp
L power:GND #PWR0161
U 1 1 610610C9
P 5500 4100
F 0 "#PWR0161" H 5500 3850 50  0001 C CNN
F 1 "GND" V 5500 3900 50  0000 C CNN
F 2 "" H 5500 4100 50  0001 C CNN
F 3 "" H 5500 4100 50  0001 C CNN
	1    5500 4100
	0    -1   -1   0   
$EndComp
$Comp
L power:GND #PWR0162
U 1 1 61061941
P 5500 4200
F 0 "#PWR0162" H 5500 3950 50  0001 C CNN
F 1 "GND" V 5500 4000 50  0000 C CNN
F 2 "" H 5500 4200 50  0001 C CNN
F 3 "" H 5500 4200 50  0001 C CNN
	1    5500 4200
	0    -1   -1   0   
$EndComp
$Comp
L Device:C_Small C18
U 1 1 6106FE9E
P 6150 4100
F 0 "C18" V 5921 4100 50  0000 C CNN
F 1 "100n" V 6012 4100 50  0000 C CNN
F 2 "Resistor_SMD:R_0603_1608Metric" H 6150 4100 50  0001 C CNN
F 3 "~" H 6150 4100 50  0001 C CNN
	1    6150 4100
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR0165
U 1 1 6106FEA4
P 6150 4200
F 0 "#PWR0165" H 6150 3950 50  0001 C CNN
F 1 "GND" H 6155 4027 50  0000 C CNN
F 2 "" H 6150 4200 50  0001 C CNN
F 3 "" H 6150 4200 50  0001 C CNN
	1    6150 4200
	1    0    0    -1  
$EndComp
$Comp
L power:+3.3V #PWR0166
U 1 1 6106FEAA
P 6150 4000
F 0 "#PWR0166" H 6150 3850 50  0001 C CNN
F 1 "+3.3V" H 6165 4173 50  0000 C CNN
F 2 "" H 6150 4000 50  0001 C CNN
F 3 "" H 6150 4000 50  0001 C CNN
	1    6150 4000
	1    0    0    -1  
$EndComp
$Comp
L Device:C_Small C19
U 1 1 61093E5D
P 8950 5900
F 0 "C19" V 8721 5900 50  0000 C CNN
F 1 "22u" V 8812 5900 50  0000 C CNN
F 2 "Capacitor_SMD:C_0805_2012Metric" H 8950 5900 50  0001 C CNN
F 3 "~" H 8950 5900 50  0001 C CNN
	1    8950 5900
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR0171
U 1 1 61093E63
P 8950 6000
F 0 "#PWR0171" H 8950 5750 50  0001 C CNN
F 1 "GND" H 8955 5827 50  0000 C CNN
F 2 "" H 8950 6000 50  0001 C CNN
F 3 "" H 8950 6000 50  0001 C CNN
	1    8950 6000
	1    0    0    -1  
$EndComp
$Comp
L power:+3.3V #PWR0172
U 1 1 61093E69
P 8950 5800
F 0 "#PWR0172" H 8950 5650 50  0001 C CNN
F 1 "+3.3V" H 8965 5973 50  0000 C CNN
F 2 "" H 8950 5800 50  0001 C CNN
F 3 "" H 8950 5800 50  0001 C CNN
	1    8950 5800
	1    0    0    -1  
$EndComp
$Comp
L Device:C_Small C20
U 1 1 61097B5B
P 9350 5900
F 0 "C20" V 9121 5900 50  0000 C CNN
F 1 "22u" V 9212 5900 50  0000 C CNN
F 2 "Capacitor_SMD:C_0805_2012Metric" H 9350 5900 50  0001 C CNN
F 3 "~" H 9350 5900 50  0001 C CNN
	1    9350 5900
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR0173
U 1 1 61097B61
P 9350 6000
F 0 "#PWR0173" H 9350 5750 50  0001 C CNN
F 1 "GND" H 9355 5827 50  0000 C CNN
F 2 "" H 9350 6000 50  0001 C CNN
F 3 "" H 9350 6000 50  0001 C CNN
	1    9350 6000
	1    0    0    -1  
$EndComp
$Comp
L power:+3.3V #PWR0174
U 1 1 61097B67
P 9350 5800
F 0 "#PWR0174" H 9350 5650 50  0001 C CNN
F 1 "+3.3V" H 9365 5973 50  0000 C CNN
F 2 "" H 9350 5800 50  0001 C CNN
F 3 "" H 9350 5800 50  0001 C CNN
	1    9350 5800
	1    0    0    -1  
$EndComp
$Comp
L Device:C_Small C21
U 1 1 610A42AF
P 5400 3800
F 0 "C21" V 5171 3800 50  0000 C CNN
F 1 "22u" V 5262 3800 50  0000 C CNN
F 2 "Capacitor_SMD:C_0805_2012Metric" H 5400 3800 50  0001 C CNN
F 3 "~" H 5400 3800 50  0001 C CNN
	1    5400 3800
	0    1    1    0   
$EndComp
$Comp
L power:GND #PWR0175
U 1 1 610A552B
P 5500 3800
F 0 "#PWR0175" H 5500 3550 50  0001 C CNN
F 1 "GND" V 5500 3600 50  0000 C CNN
F 2 "" H 5500 3800 50  0001 C CNN
F 3 "" H 5500 3800 50  0001 C CNN
	1    5500 3800
	0    -1   -1   0   
$EndComp
Wire Wire Line
	5300 3800 5200 3800
$Comp
L Device:RF_Shield_One_Piece SH1
U 1 1 610DBF58
P 6750 5650
F 0 "SH1" H 7380 5639 50  0000 L CNN
F 1 "DCXO Shield" H 7380 5548 50  0000 L CNN
F 2 "RF_Shielding:Laird_Technologies_BMI-S-102_16.50x16.50mm" H 6750 5550 50  0001 C CNN
F 3 "~" H 6750 5550 50  0001 C CNN
	1    6750 5650
	1    0    0    -1  
$EndComp
$Comp
L power:+3.3V #PWR0177
U 1 1 610E291C
P 6750 6050
F 0 "#PWR0177" H 6750 5900 50  0001 C CNN
F 1 "+3.3V" V 6750 6300 50  0000 C CNN
F 2 "" H 6750 6050 50  0001 C CNN
F 3 "" H 6750 6050 50  0001 C CNN
	1    6750 6050
	-1   0    0    1   
$EndComp
$Comp
L Device:R_Small_US R11
U 1 1 61086ABE
P 9950 1500
F 0 "R11" V 9745 1500 50  0000 C CNN
F 1 "15k" V 9836 1500 50  0000 C CNN
F 2 "Resistor_SMD:R_0603_1608Metric" H 9950 1500 50  0001 C CNN
F 3 "~" H 9950 1500 50  0001 C CNN
	1    9950 1500
	0    -1   1    0   
$EndComp
$Comp
L power:GND #PWR0178
U 1 1 610870A0
P 10050 1500
F 0 "#PWR0178" H 10050 1250 50  0001 C CNN
F 1 "GND" V 10050 1300 50  0000 C CNN
F 2 "" H 10050 1500 50  0001 C CNN
F 3 "" H 10050 1500 50  0001 C CNN
	1    10050 1500
	0    -1   -1   0   
$EndComp
Wire Wire Line
	9850 1500 9800 1500
Connection ~ 9800 1500
$Comp
L Device:C_Small C23
U 1 1 610AF32A
P 9700 750
F 0 "C23" V 9500 750 50  0000 C CNN
F 1 "1n" V 9600 750 50  0000 C CNN
F 2 "Resistor_SMD:R_0603_1608Metric" H 9700 750 50  0001 C CNN
F 3 "~" H 9700 750 50  0001 C CNN
	1    9700 750 
	-1   0    0    1   
$EndComp
$Comp
L power:GND #PWR0179
U 1 1 610CC665
P 9700 650
F 0 "#PWR0179" H 9700 400 50  0001 C CNN
F 1 "GND" H 9705 477 50  0000 C CNN
F 2 "" H 9700 650 50  0001 C CNN
F 3 "" H 9700 650 50  0001 C CNN
	1    9700 650 
	-1   0    0    1   
$EndComp
Wire Wire Line
	3900 4100 3900 5050
Wire Wire Line
	4000 4000 4000 4950
Wire Wire Line
	3900 5050 3100 5050
Wire Wire Line
	1200 4200 1200 5050
Wire Wire Line
	1300 4950 3200 4950
Wire Wire Line
	1300 4100 1300 4950
Connection ~ 3200 4950
Wire Wire Line
	3200 4950 4000 4950
Connection ~ 3100 5050
Wire Wire Line
	3100 5050 3100 6850
Wire Wire Line
	3100 5050 1200 5050
Text Label 3500 5050 0    50   ~ 0
SDA
Text Label 3500 4950 0    50   ~ 0
SCL
$Comp
L power:GND #PWR0117
U 1 1 610C1E00
P 4700 4300
F 0 "#PWR0117" H 4700 4050 50  0001 C CNN
F 1 "GND" V 4700 4100 50  0000 C CNN
F 2 "" H 4700 4300 50  0001 C CNN
F 3 "" H 4700 4300 50  0001 C CNN
	1    4700 4300
	0    1    1    0   
$EndComp
$Comp
L power:+3.3V #PWR0119
U 1 1 610D491E
P 9400 1700
F 0 "#PWR0119" H 9400 1550 50  0001 C CNN
F 1 "+3.3V" V 9400 1950 50  0000 C CNN
F 2 "" H 9400 1700 50  0001 C CNN
F 3 "" H 9400 1700 50  0001 C CNN
	1    9400 1700
	0    1    1    0   
$EndComp
$Comp
L power:+3.3V #PWR0123
U 1 1 610D5D8F
P 9400 2700
F 0 "#PWR0123" H 9400 2550 50  0001 C CNN
F 1 "+3.3V" V 9400 2950 50  0000 C CNN
F 2 "" H 9400 2700 50  0001 C CNN
F 3 "" H 9400 2700 50  0001 C CNN
	1    9400 2700
	0    1    1    0   
$EndComp
$Comp
L power:+3.3V #PWR0126
U 1 1 610D62D1
P 9400 2600
F 0 "#PWR0126" H 9400 2450 50  0001 C CNN
F 1 "+3.3V" V 9400 2850 50  0000 C CNN
F 2 "" H 9400 2600 50  0001 C CNN
F 3 "" H 9400 2600 50  0001 C CNN
	1    9400 2600
	0    1    1    0   
$EndComp
$Comp
L power:+3.3V #PWR0127
U 1 1 610D67E1
P 9400 3000
F 0 "#PWR0127" H 9400 2850 50  0001 C CNN
F 1 "+3.3V" V 9400 3250 50  0000 C CNN
F 2 "" H 9400 3000 50  0001 C CNN
F 3 "" H 9400 3000 50  0001 C CNN
	1    9400 3000
	0    1    1    0   
$EndComp
Text Label 1400 3500 1    50   ~ 0
gRST
Text Label 1500 6950 3    50   ~ 0
gANT
$Comp
L power:+3.3V #PWR0128
U 1 1 610F092A
P 1600 2200
F 0 "#PWR0128" H 1600 2050 50  0001 C CNN
F 1 "+3.3V" V 1600 2450 50  0000 C CNN
F 2 "" H 1600 2200 50  0001 C CNN
F 3 "" H 1600 2200 50  0001 C CNN
	1    1600 2200
	0    -1   -1   0   
$EndComp
$Comp
L power:+3.3V #PWR0129
U 1 1 610F0CBB
P 1600 2300
F 0 "#PWR0129" H 1600 2150 50  0001 C CNN
F 1 "+3.3V" V 1600 2550 50  0000 C CNN
F 2 "" H 1600 2300 50  0001 C CNN
F 3 "" H 1600 2300 50  0001 C CNN
	1    1600 2300
	0    -1   -1   0   
$EndComp
$Comp
L power:+3.3V #PWR0130
U 1 1 610F0FB9
P 1600 2400
F 0 "#PWR0130" H 1600 2250 50  0001 C CNN
F 1 "+3.3V" V 1600 2650 50  0000 C CNN
F 2 "" H 1600 2400 50  0001 C CNN
F 3 "" H 1600 2400 50  0001 C CNN
	1    1600 2400
	0    -1   -1   0   
$EndComp
$Comp
L power:+3.3V #PWR0131
U 1 1 610F128D
P 1600 2500
F 0 "#PWR0131" H 1600 2350 50  0001 C CNN
F 1 "+3.3V" V 1600 2750 50  0000 C CNN
F 2 "" H 1600 2500 50  0001 C CNN
F 3 "" H 1600 2500 50  0001 C CNN
	1    1600 2500
	0    -1   -1   0   
$EndComp
$Comp
L power:+3.3V #PWR0118
U 1 1 610D233B
P 9400 2500
F 0 "#PWR0118" H 9400 2350 50  0001 C CNN
F 1 "+3.3V" V 9400 2750 50  0000 C CNN
F 2 "" H 9400 2500 50  0001 C CNN
F 3 "" H 9400 2500 50  0001 C CNN
	1    9400 2500
	0    1    1    0   
$EndComp
$Comp
L power:+3.3V #PWR0122
U 1 1 610D583A
P 9400 1200
F 0 "#PWR0122" H 9400 1050 50  0001 C CNN
F 1 "+3.3V" V 9400 1450 50  0000 C CNN
F 2 "" H 9400 1200 50  0001 C CNN
F 3 "" H 9400 1200 50  0001 C CNN
	1    9400 1200
	0    1    1    0   
$EndComp
$Comp
L power:+3.3V #PWR0104
U 1 1 611868C4
P 9400 1800
F 0 "#PWR0104" H 9400 1650 50  0001 C CNN
F 1 "+3.3V" V 9400 2050 50  0000 C CNN
F 2 "" H 9400 1800 50  0001 C CNN
F 3 "" H 9400 1800 50  0001 C CNN
	1    9400 1800
	0    1    1    0   
$EndComp
$Comp
L power:+3.3V #PWR0109
U 1 1 61186E59
P 9400 1300
F 0 "#PWR0109" H 9400 1150 50  0001 C CNN
F 1 "+3.3V" V 9400 1550 50  0000 C CNN
F 2 "" H 9400 1300 50  0001 C CNN
F 3 "" H 9400 1300 50  0001 C CNN
	1    9400 1300
	0    1    1    0   
$EndComp
$Comp
L power:+3.3V #PWR0120
U 1 1 6118ECD9
P 9400 1600
F 0 "#PWR0120" H 9400 1450 50  0001 C CNN
F 1 "+3.3V" V 9400 1850 50  0000 C CNN
F 2 "" H 9400 1600 50  0001 C CNN
F 3 "" H 9400 1600 50  0001 C CNN
	1    9400 1600
	0    1    1    0   
$EndComp
Text GLabel 9850 5350 2    50   Input ~ 0
PPS
Text GLabel 9850 5250 2    50   Input ~ 0
CLK
$Comp
L power:+3.3V #PWR0114
U 1 1 611DA105
P 1600 1900
F 0 "#PWR0114" H 1600 1750 50  0001 C CNN
F 1 "+3.3V" V 1600 2150 50  0000 C CNN
F 2 "" H 1600 1900 50  0001 C CNN
F 3 "" H 1600 1900 50  0001 C CNN
	1    1600 1900
	0    -1   -1   0   
$EndComp
$Comp
L power:+3.3V #PWR0121
U 1 1 611DA46C
P 1600 1600
F 0 "#PWR0121" H 1600 1450 50  0001 C CNN
F 1 "+3.3V" V 1600 1850 50  0000 C CNN
F 2 "" H 1600 1600 50  0001 C CNN
F 3 "" H 1600 1600 50  0001 C CNN
	1    1600 1600
	0    -1   -1   0   
$EndComp
Wire Wire Line
	1400 5700 1400 2800
Wire Wire Line
	1400 2800 1600 2800
$Comp
L power:+3.3V #PWR0132
U 1 1 61108331
P 9400 2400
F 0 "#PWR0132" H 9400 2250 50  0001 C CNN
F 1 "+3.3V" V 9400 2650 50  0000 C CNN
F 2 "" H 9400 2400 50  0001 C CNN
F 3 "" H 9400 2400 50  0001 C CNN
	1    9400 2400
	0    1    1    0   
$EndComp
$Comp
L power:+3.3V #PWR0133
U 1 1 611085DD
P 9400 2300
F 0 "#PWR0133" H 9400 2150 50  0001 C CNN
F 1 "+3.3V" V 9400 2550 50  0000 C CNN
F 2 "" H 9400 2300 50  0001 C CNN
F 3 "" H 9400 2300 50  0001 C CNN
	1    9400 2300
	0    1    1    0   
$EndComp
$EndSCHEMATC
