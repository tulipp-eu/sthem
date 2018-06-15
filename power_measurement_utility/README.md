# Power Measurement Utility

The Lynsyn PMU is a board used to sample both current and program counter from a Zynq based development board.  Lynsyn is designed to work with the TULIPP analysis_utility.

## HW Installation

1. Go to the power_measurement_utility directory
2. sudo make install_hw

## Compiling Firmware and Support SW

Requirements:
* SiLabs gecko SDK (get it by installing SimplicityStudio)
* Xilinx FPGA development tools with support for Artix devices
* libusb-1.0 (sudo apt-get install libusb-1.0-0-dev)

Instructions:
1. Go to the power_measurement_utility directory
2. Edit the Makefile such that SDK points to your gecko SDK directory
3. make

The resulting binaries are put in analysis_utility/bin.

## Running the Test and Calibration Program

The test and calibration program is used to flash the firmware (both MCU and FPGA) to the board and run some board test and calibration procedures.

Requirements:
* Xilinx FPGA development tools in the path.  Tested with SDSoC 2017.2. 
* Xilinx USB cable (or compatible device)
* SiLabs Simplicity Commander in the path.  https://www.silabs.com/products/mcu/programming-options
* EFM32 development board capabale of programming external devices
* JTAG cable (Xilinx 14 pin)

Instructions:
1. Run the bin/lynsyn_tester binary and follow the on-screen instructions
