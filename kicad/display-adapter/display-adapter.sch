EESchema Schematic File Version 4
EELAYER 30 0
EELAYER END
$Descr A4 11693 8268
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
L Connector:Conn_01x14_Female J2
U 1 1 5FCC0130
P 6850 3450
F 0 "J2" H 6878 3426 50  0000 L CNN
F 1 "Conn_01x14_Female" H 6878 3335 50  0000 L CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x14_P2.54mm_Vertical" H 6850 3450 50  0001 C CNN
F 3 "~" H 6850 3450 50  0001 C CNN
	1    6850 3450
	1    0    0    -1  
$EndComp
$Comp
L Connector_Generic:Conn_02x07_Odd_Even J1
U 1 1 5FCC37ED
P 4750 3500
F 0 "J1" H 4800 4017 50  0000 C CNN
F 1 "Conn_02x07_Odd_Even" H 4800 3926 50  0000 C CNN
F 2 "Connector_IDC:IDC-Header_2x07_P2.54mm_Vertical" H 4750 3500 50  0001 C CNN
F 3 "~" H 4750 3500 50  0001 C CNN
	1    4750 3500
	1    0    0    -1  
$EndComp
Wire Wire Line
	6650 3050 6200 3050
Wire Wire Line
	6650 3150 6200 3150
Wire Wire Line
	6650 3250 6200 3250
Wire Wire Line
	6650 3350 6200 3350
Wire Wire Line
	6650 3450 6200 3450
Wire Wire Line
	6650 3550 6200 3550
Wire Wire Line
	6650 3650 6200 3650
Wire Wire Line
	6650 3750 6200 3750
Wire Wire Line
	6650 3850 6200 3850
Wire Wire Line
	6650 3950 6200 3950
Wire Wire Line
	6650 4050 6200 4050
Wire Wire Line
	6650 4150 6200 4150
Wire Wire Line
	5050 3300 5500 3300
Wire Wire Line
	5050 3400 5500 3400
Wire Wire Line
	5050 3500 5500 3500
Wire Wire Line
	5050 3600 5500 3600
Wire Wire Line
	5050 3700 5500 3700
Wire Wire Line
	5050 3800 5500 3800
Wire Wire Line
	4550 3200 4050 3200
Wire Wire Line
	4550 3300 4050 3300
Wire Wire Line
	4550 3400 4050 3400
Wire Wire Line
	4550 3500 4050 3500
Wire Wire Line
	4550 3600 4050 3600
Wire Wire Line
	4550 3700 4050 3700
Wire Wire Line
	4550 3800 4050 3800
$Comp
L power:+5V #PWR0101
U 1 1 5FCD1FCD
P 4050 2950
F 0 "#PWR0101" H 4050 2800 50  0001 C CNN
F 1 "+5V" H 4065 3123 50  0000 C CNN
F 2 "" H 4050 2950 50  0001 C CNN
F 3 "" H 4050 2950 50  0001 C CNN
	1    4050 2950
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR0102
U 1 1 5FCD36A4
P 5650 3250
F 0 "#PWR0102" H 5650 3000 50  0001 C CNN
F 1 "GND" H 5655 3077 50  0000 C CNN
F 2 "" H 5650 3250 50  0001 C CNN
F 3 "" H 5650 3250 50  0001 C CNN
	1    5650 3250
	1    0    0    -1  
$EndComp
Wire Wire Line
	5650 3200 5650 3250
Wire Wire Line
	5050 3200 5650 3200
Wire Wire Line
	4050 2950 4050 3200
$Comp
L power:+5V #PWR0103
U 1 1 5FCD5326
P 6000 2800
F 0 "#PWR0103" H 6000 2650 50  0001 C CNN
F 1 "+5V" H 6015 2973 50  0000 C CNN
F 2 "" H 6000 2800 50  0001 C CNN
F 3 "" H 6000 2800 50  0001 C CNN
	1    6000 2800
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR0104
U 1 1 5FCD6035
P 6000 3000
F 0 "#PWR0104" H 6000 2750 50  0001 C CNN
F 1 "GND" H 6005 2827 50  0000 C CNN
F 2 "" H 6000 3000 50  0001 C CNN
F 3 "" H 6000 3000 50  0001 C CNN
	1    6000 3000
	1    0    0    -1  
$EndComp
Wire Wire Line
	6000 2850 6000 2800
Wire Wire Line
	6000 2850 6650 2850
Wire Wire Line
	6000 2950 6000 3000
Wire Wire Line
	6000 2950 6650 2950
Text Label 4050 3300 0    50   ~ 0
TFT_CS
Text Label 4050 3400 0    50   ~ 0
TFT_DC
Text Label 4050 3500 0    50   ~ 0
SCLK
Text Label 4050 3600 0    50   ~ 0
MISO
Text Label 4050 3700 0    50   ~ 0
TCH_CS
Text Label 4050 3800 0    50   ~ 0
MISO
Text Label 5500 3300 2    50   ~ 0
TFT_RST
Text Label 5500 3400 2    50   ~ 0
MOSI
Text Label 5500 3500 2    50   ~ 0
TFT_LED
Text Label 5500 3600 2    50   ~ 0
SCLK
Text Label 5500 3700 2    50   ~ 0
MOSI
Text Label 5500 3800 2    50   ~ 0
TCH_IRQ
Text Label 6200 3050 0    50   ~ 0
TFT_CS
Text Label 6200 3150 0    50   ~ 0
TFT_RST
Text Label 6200 3250 0    50   ~ 0
TFT_DC
Text Label 6200 3350 0    50   ~ 0
MOSI
Text Label 6200 3450 0    50   ~ 0
SCLK
Text Label 6200 3550 0    50   ~ 0
TFT_LED
Text Label 6200 3650 0    50   ~ 0
MISO
Text Label 6200 3750 0    50   ~ 0
SCLK
Text Label 6200 3850 0    50   ~ 0
TCH_CS
Text Label 6200 3950 0    50   ~ 0
MOSI
Text Label 6200 4050 0    50   ~ 0
MISO
Text Label 6200 4150 0    50   ~ 0
TCH_IRQ
$EndSCHEMATC
