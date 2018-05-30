# Analysis Utility

## Build Instructions

Requirements:
* Qt5.9 with charts and serialport
* libusb-1.0
* clang (tested with v4.0, v5.0 and v6.0)

Requirements are met on Ubuntu 18.04 with the following command:
* sudo apt install build-essential qt5-default libqt5charts5-dev libqt5serialport5-dev libusb-1.0-0-dev clang-6.0 libclang-6.0-dev

Requirements are met on Ubuntu 17.10 and Ubuntu 16.04 with the following command:
* sudo apt install build-essential qt5-default libqt5charts5-dev libqt5serialport5-dev libusb-1.0-0-dev clang-5.0 libclang-5.0-dev

Instructions:
1. Enter the analysis_utility directory
2. Edit the Makefile so that the variables at the top correspond to your environment
3. make

The resulting binaries are put in analysis_utility/bin.

See the [wiki](https://github.com/tulipp-eu/sthem/wiki) for usage information.
