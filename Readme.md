# RP2040 IR receiver
> Implementing a USB IR receiver using the RP2040 microcontroller

## Introduction
This small "lazy sunday" project was created with the goal to get familiar with the Raspberry Pi Pico SDK.\
Do not expect anything fancy here, it implements a simple IR receiver for NEC remotes as a custom USB device that also contains a WCID so no drivers if you are using this device with Windows. On Linux it should be possible to access it via libUSB, however I have not tested that.

## Hardware
You need a RP2040 based board and a TL1838 IR receiver, thats all. Make sure to power the TL1838 from 3V3, not 5V.

## Perquisites
- The [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk) incl. all necessary tools (cmake, compiler, etc.)
- Optional: Python 3 and the `winusbcdc` package if you want to use the example script (Windows only)

## Build
1. `cmake -G Ninja -B build`
2. `cmake --build build`

## Usage
### Hardware Setup
See the `Main.cpp` in the `src` directory on how to change to code to support different hardware setups. Currently a (optional) normal LED or a WS2812B LED can be used as status display.
### Read IR Code
See the `usbtest.py` script on how to get the decoded IR code on the PC side.