# AVR-NET-IO-Smartmeter
A simple smartmeter which is connected to a S0-interface and counts the energy consumption or provides the current power consumption. The informations can be read via REST-API.

It runs on the [Pollin AVR-NET-IO](https://www.pollin.de/p/avr-net-io-fertigmodul-810073) board.

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](http://choosealicense.com/licenses/mit/)
[![Repo Status](https://www.repostatus.org/badges/latest/inactive.svg)](https://www.repostatus.org/#inactive)
[![Release](https://img.shields.io/github/release/BlueAndi/avr-net-io-smartmeter.svg)](https://github.com/BlueAndi/avr-net-io-smartmeter/releases)
[![PlatformIO CI](https://github.com/BlueAndi/avr-net-io-smartmeter/workflows/PlatformIO%20CI/badge.svg?branch=main)](https://github.com/BlueAndi/avr-net-io-smartmeter/actions?query=workflow%3A%22PlatformIO+CI%22)

* [AVR-NET-IO-Smartmeter](#avr-net-io-smartmeter)
* [Motivation](#motivation)
* [Usage](#usage)
* [Electronic](#electronic)
* [Software](#software)
  * [IDE](#ide)
  * [Installation](#installation)
  * [Install bootloader](#install-bootloader)
  * [Change MAC address of network interface controller](#change-mac-address-of-network-interface-controller)
  * [Build Project](#build-project)
  * [Update of the device](#update-of-the-device)
    * [Update via serial interface](#update-via-serial-interface)
  * [Used Libraries](#used-libraries)
* [REST API](#rest-api)
  * [Get data from one single S0 interface (GET /api/s0-interface/\<s0-interface-id\>)](#get-data-from-one-single-s0-interface-get-apis0-interfaces0-interface-id)
  * [Get data from all S0 interfaces at once (GET /api/s0-interfaces)](#get-data-from-all-s0-interfaces-at-once-get-apis0-interfaces)
* [Issues, Ideas And Bugs](#issues-ideas-and-bugs)
* [License](#license)
* [Contribution](#contribution)

# Motivation
The idea was to have a simple way to get the power consumption of the heatpump and the rest of the house. The data shall be provided over a REST-API, which can easily be used from e.g. bash via curl. The data is retrieved periodically, pushed to a [influx-database](https://www.influxdata.com/) and visualized with [grafana](https://grafana.com/).

Up to 8 S0 interface would be possible with the AVR-NET-IO board, but because of RAM limitation only 2 are configured. More may be possible, but that was never verified.

# Usage

1. Connect your S0 signal with the board, see the table in the next chapter.
2. Configure S0 interface (0-1) by browsing to http://&lt;device-ip-address&gt;/configure/&lt;s0-interface&gt; with your favorite browser. Replace &lt;s0-interface&gt; with the S0 interface id.
3. Configure the S0 interface and enable it.

# Electronic

* [Pollin AVR-NET-IO](https://www.pollin.de/p/avr-net-io-fertigmodul-810073)

Note, the AVR-NET-IO has original a ATmega32-16PU, which was replaced with a ATmega644p.

> [!WARNING]  
> 2024-01-01: The Pollin AVR-NET-IO development is obsolete and not available anymore.\
> There is no known successor development board.

The following table shows the possible connections for S0 signals.

| Terminal | Name | MCU Pin | Arduino Pin |
| -------- | ---- |-------- | ----------- |
| J3 | Input 1 | PA0 | 24 |
| J3 | Input 2 | PA1 | 25 |
| J3 | Input 3 | PA2 | 26 |
| J3 | Input 4 | PA3 | 27 |
| J9 | ADC1 | PA4 | 28 |
| J9 | ADC2 | PA5 | 29 |
| J7 | ADC3 | PA6 | 30 |
| J7 | ADC4 | PA7 | 31 |

# Software

## IDE
The [PlatformIO IDE](https://platformio.org/platformio-ide) is used for the development. Its build on top of Microsoft Visual Studio Code.

## Installation
1. Install [VSCode](https://code.visualstudio.com/).
2. Install PlatformIO IDE according to this [HowTo](https://platformio.org/install/ide?install=vscode).
3. Close and start VSCode again.
4. Recommended is to take a look to the [quick-start guide](https://docs.platformio.org/en/latest/ide/vscode.html#quick-start).

## Install bootloader
Note, for the following steps I used the AtmelStudio v7.0 with my AVR-ISP programmer, because I didn't manage to get this setup work with VSCode + Platformio or the Arduino IDE.
1. Set fuse bits: lfuse = 0xf7, hfuse = 0xd6, efuse = 0xfd
2. The MightyCore provides in the platformio installation directy bootloaders. Choose ```.platformio/packages/framework-arduino-avr-mightycore/bootloaders/optiboot_flash/bootloaders/atmega644p/16000000L/optiboot_flash_atmega644p_UART0_115200_16000000L_B0_BIGBOOT.hex```
3. If the bootloader is active, two pulses are shown on pin B0 (= Arduino pin 0), which can be checked with a oscilloscope.

After the bootloader is installed and running, the typical Arduino upload can be used over the serial interface.

## Change MAC address of network interface controller
Every AVR-NET-IO board comes with a unique MAC address of the network interface controller. Before you build the software it is necessary to set it in the sourcecode. Therefore open ```./src/main.cpp``` in the editor, search for the variable ```DEVICE_MAC_ADDR``` and change it accordingly.

## Build Project
1. Load workspace in VSCode.
2. Change to PlatformIO toolbar.
3. _Project Tasks -> Build All_ or via hotkey ctrl-alt-b

## Update of the device

### Update via serial interface
1. Connect the AVR-NET-IO board to your PC via serial interface.
2. Build and upload the software via _Project Tasks -> Upload All_.
3. Note, if the AVR-NET-IO board is not modified, you need to keep it off until in the console ```Uploading .pio\build\MightyCore\firmware.hex``` is shown. Just in this moment power the board and the upload starts.

## Used Libraries
* [MightyCore](https://github.com/MCUdude/MightyCore) - Arduino core for ATmega644.
* [EthernetENC](https://github.com/jandrassy/EthernetENC) - Ethernet library for ENC28J60 with Arduino compatible interface.
* [ArduinoHttpServer](https://github.com/QuickSander/ArduinoHttpServer) - Server side minimalistic object oriented HTTP protocol implementation.
* [ArduinoJSON](https://arduinojson.org/) - JSON library.

# REST API

## Get data from one single S0 interface (GET /api/s0-interface/&lt;s0-interface-id&gt;)
Get S0 interface data:
* S0 interface unique id.
* S0 interface name.
* The current power consumption in W.
* Number of counted pulses.
* Energy consumption in Wh, depended on the number of counted pulses.

```<s0-interface-id>```:
* The S0 interface id is in range [0; 1].

Response:
```json
{
  "data": {
    "id": 0,
    "name": "S0-0",
    "pulsesPer1KWh": 1000,
    "powerConsumption": 230,
    "pulses": 40,
    "energyConsumption": 460
  },
  "status":0
}
```

Status 0 means successful. If the request fails, it the status will be non-zero and data is empty.

## Get data from all S0 interfaces at once (GET /api/s0-interfaces)
Get data from all S0 interfaces at once.

A S0 interface contains the following data:
* S0 interface unique id.
* S0 interface name.
* The current power consumption in W.
* Number of counted pulses.
* Energy consumption in Wh, depended on the number of counted pulses.

Response:
```json
{
  "data": [{
    "id": 0,
    "name": "S0-0",
    "pulsesPer1KWh": 1000,
    "powerConsumption": 230,
    "pulses": 40,
    "energyConsumption": 460
  }, {
    "id": 1,
    "name": "S0-1",
    "pulsesPer1KWh": 1000,
    "powerConsumption": 50,
    "pulses": 20,
    "energyConsumption": 100
  }],
  "status":0
}
```

Status 0 means successful. If the request fails, it the status will be non-zero and data is empty.

# Issues, Ideas And Bugs
If you have further ideas or you found some bugs, great! Create a [issue](https://github.com/BlueAndi/avr-net-io-smartmeter/issues) or if you are able and willing to fix it by yourself, clone the repository and create a pull request.

# License
The whole source code is published under the [MIT license](http://choosealicense.com/licenses/mit/).
Consider the different licenses of the used third party libraries too!

# Contribution
Unless you explicitly state otherwise, any contribution intentionally submitted for inclusion in the work by you, shall be licensed as above, without any
additional terms or conditions.
