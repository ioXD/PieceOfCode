This program get signal strength of antenna Ubiquiti Nano Loco M5 using 4 gpios of 
router TLMR3420 with OpenWrt Chaos Calmer. The antenna should find other signal and 
looking the best position where the signal is more strength. First, scan 0-180° and 
set the signal strength, After search this signal and try to keep this posistion.

To compile only is necessary the gcc compiler:

gcc signalTrackerCode.c -o signalTrackerCode
