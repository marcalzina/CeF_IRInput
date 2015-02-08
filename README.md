# The CeF_IRInput library

## Description

Written by Marc Alzina

MIT License, check license.txt for more information.

This library is for receiving and decoding codes from an infra-red remote control using an Arduino.

## Portability

This code was developed and tested using an Arduino Nano (ATmega328, 16MHz) but I believe it should work with no or little modification on other Arduinos (see PinMonitor::getInterruptId()).

## Installation

To install, download the zip file, unzip and rename the folder to "CeF_IRInput", copy it in your Arduino libraries directories and restart your Arduino IDE. Look into File/Examples/CeF_IRInput/IRDumpCode.

## Miscellaneous

* This library keeps recording data even while decoding it so that it never misses a single command, provided of course that the buffer is big enough and that your program does not block interrupts for too long.
* **Precision of the measure**: If you want to measure the timings precisely, use ParamsHighRes instead of ParamsLowRes. I have compared the results to what my USB Saleae 24 Mhz 8 Channel Logic Analyzer returns, and the difference is less than 4 microseconds, which is the precision of the micros() function on this Arduino.
* The fact that we use interrupt to get the signal makes it precise and avoids wasting CPU cycles pooling the pin state. Nevertheless it has the drawback of limiting which pins can be used since not all of the digital input pins can trigger interrupts (there are only 2 on the Nano). This may be a problem depending on your project.
* FYI, **CeF** stands for "Comprendre Et Faire", an hopefully unique prefix to avoid name clashes.

## To do

* Support more remote-control protocols

## Bibliography and useful links

* [NEC Infrared Transmission Protocol](http://techdocs.altium.com/display/FPGA/NEC+Infrared+Transmission+Protocol)
* Datasheets for [IR Receiver Modules for Remote Control Systems](http://www.vishay.com/docs/82459/tsop48.pdf)
* [Ken Shirriff's Multi-Protocol Infrared Remote Library for the Arduino](http://www.righto.com/2009/08/multi-protocol-infrared-remote-library.html)
* [Arduino Nano specs](http://arduino.cc/en/Main/arduinoBoardNano) (see "External Interrupts: 2 and 3. These pins can be configured to trigger an interrupt on a low value, a rising or falling edge, or a change in value.")
