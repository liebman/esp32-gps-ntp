# This is an ESP32 NTP Server with DS3231 synched to GPS PPS signal

**NOTE: This is functional but *very much* a work in progress at the moment.**

This is an [esp-idf](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/index.html) app using [platformio](https://platformio.org/).

## Some details

Synching the DS3231 to the GPS is done with a small high level (level 5) interrupt handler in assembly.  This generates a timestamp and offset in microseconds tracking the active edges of the GPS PPS and RTC SQW signals.  This data is then fed in to a PID algorithm that will generate an offset value used to speed up and slow down the DS3231 RTC.

- [main](main) Contains the code
- [kicad/esp-gps-ntp](kicad/esp-gps-ntp) contains the schematic and board designs in KiCad.
- [kicad/display-adapter](kicad/display-adapter) contains the schematic and board design for a small adapter to config a single inline header connector to an IDC connector (for ribbon cable connection of display)

The display I'm using is [here](https://www.amazon.com/gp/product/B073R7BH1B)

## Schematic

![Schematic](images/ESPNTPServer.png)
