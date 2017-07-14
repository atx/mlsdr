EESchema Schematic File Version 2
LIBS:mlaticka-sdr-rescue
LIBS:power
LIBS:device
LIBS:transistors
LIBS:conn
LIBS:linear
LIBS:regul
LIBS:74xx
LIBS:cmos4000
LIBS:adc-dac
LIBS:memory
LIBS:xilinx
LIBS:microcontrollers
LIBS:dsp
LIBS:microchip
LIBS:analog_switches
LIBS:motorola
LIBS:texas
LIBS:intel
LIBS:audio
LIBS:interface
LIBS:digital-audio
LIBS:philips
LIBS:display
LIBS:cypress
LIBS:siliconi
LIBS:opto
LIBS:atmel
LIBS:contrib
LIBS:valves
LIBS:93lc56bt
LIBS:ltc22xx
LIBS:ncp700b
LIBS:sma
LIBS:lp5912
LIBS:r820t
LIBS:10m08scu169
LIBS:ft232h
LIBS:asv-xx
LIBS:ws2812
LIBS:mlaticka-sdr-cache
EELAYER 25 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 7
Title ""
Date ""
Rev "v1.0"
Comp "Josef Gajdusek <atx@atx.name>"
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Sheet
S 3300 1950 950  375 
U 583AF70A
F0 "ADC" 60
F1 "adc.sch" 60
F2 "IN_P" I L 3300 2100 59 
F3 "IN_N" I L 3300 2175 59 
F4 "DATA[0..13]" O R 4250 2100 59 
F5 "CLK" I R 4250 2200 59 
F6 "BYPASS" I L 3300 2275 59 
$EndSheet
$Sheet
S 7150 2000 1025 1100
U 583C3950
F0 "FTDI" 59
F1 "ftdi.sch" 59
F2 "~RXF" O L 7150 2300 59 
F3 "~TXE" O L 7150 2400 59 
F4 "~RD" I L 7150 2500 59 
F5 "~WR" I L 7150 2600 59 
F6 "CLK" O L 7150 2100 59 
F7 "~OE" I L 7150 2700 59 
F8 "RST" O L 7150 2825 59 
F9 "DATA[0..7]" B L 7150 2200 59 
$EndSheet
$Sheet
S 7550 3650 1050 750 
U 583E6FA2
F0 "FPGA-Misc" 59
F1 "fpga_misc.sch" 59
$EndSheet
$Sheet
S 4850 2000 1475 1125
U 583F04C5
F0 "FPGA" 39
F1 "fpga.sch" 39
F2 "ADC_DATA[0..13]" I L 4850 2100 59 
F3 "ADC_CLK" I L 4850 2200 59 
F4 "FT_RST" I R 6325 2825 59 
F5 "FT_~OE" I R 6325 2700 59 
F6 "FT_CLK" I R 6325 2100 59 
F7 "FT_~WR" O R 6325 2600 59 
F8 "FT_~RD" O R 6325 2500 59 
F9 "FT_~TXE" I R 6325 2400 59 
F10 "FT_~RXF" I R 6325 2300 59 
F11 "FT_DATA[0..7]" B R 6325 2200 59 
F12 "SDA" B L 4850 2750 59 
F13 "SCL" B L 4850 2625 59 
F14 "TUNER_CLK" O L 4850 2475 59 
$EndSheet
Wire Wire Line
	3300 2100 2975 2100
Wire Wire Line
	2975 2175 3300 2175
$Sheet
S 2450 4650 525  450 
U 583F4127
F0 "Analog-PSU" 59
F1 "analog_psu.sch" 59
$EndSheet
Wire Bus Line
	4250 2100 4850 2100
Wire Wire Line
	4850 2200 4250 2200
Wire Wire Line
	7150 2825 6325 2825
Wire Wire Line
	6325 2700 7150 2700
Wire Wire Line
	7150 2100 6325 2100
Wire Wire Line
	6325 2600 7150 2600
Wire Wire Line
	7150 2500 6325 2500
Wire Wire Line
	6325 2400 7150 2400
Wire Wire Line
	6325 2300 7150 2300
Wire Bus Line
	7150 2200 6325 2200
Wire Wire Line
	2975 2750 4850 2750
Wire Wire Line
	4850 2625 2975 2625
Wire Wire Line
	4850 2475 2975 2475
$Sheet
S 2425 2000 550  1000
U 583AC59E
F0 "RF" 60
F1 "rf.sch" 60
F2 "SCL" B R 2975 2625 59 
F3 "SDA" B R 2975 2750 59 
F4 "CLK" I R 2975 2475 59 
F5 "IF_N" O R 2975 2175 59 
F6 "IF_P" O R 2975 2100 59 
F7 "BYPASS" O R 2975 2275 59 
$EndSheet
Wire Wire Line
	3300 2275 2975 2275
$EndSCHEMATC
