/******************************************************************************
 *
 *  This file is part of the TULIPP Lynsyn Power Measurement Utilitity
 *
 *  Copyright 2018 Asbjørn Djupdal, NTNU, TULIPP EU Project
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

///////////////////////////////////////////////////////////////////////////////
//
// Initialization, test and calibration program for Lynsyn V2.0 and 2.1
//
///////////////////////////////////////////////////////////////////////////////

#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <libusb.h>

#include "../mcu/usbprotocol.h"

///////////////////////////////////////////////////////////////////////////////
// uncomment for Lynsyn V2.0, comment for Lynsyn V2.1

//#define VERSION_20

///////////////////////////////////////////////////////////////////////////////

#define LYNSYN_MAX_CURRENT_VALUE 32768
#define LYNSYN_REF_VOLTAGE 2.5

#ifdef VERSION_20

#define LYNSYN_RS 8200

#else

double rlDefault[7] = {0.025, 0.05, 0.05, 0.1, 0.1, 1, 10};

#endif

///////////////////////////////////////////////////////////////////////////////

struct libusb_device_handle *lynsynHandle;
uint8_t outEndpoint;
uint8_t inEndpoint;
struct libusb_context *usbContext;
libusb_device **devs;

///////////////////////////////////////////////////////////////////////////////

void sendBytes(uint8_t *bytes, int numBytes) {
  int remaining = numBytes;
  int transfered = 0;
  while(remaining > 0) {
    libusb_bulk_transfer(lynsynHandle, outEndpoint, bytes, numBytes, &transfered, 0);
    remaining -= transfered;
    bytes += transfered;
  }
}

void getBytes(uint8_t *bytes, int numBytes) {
  int transfered = 0;
  int ret = 0;

  ret = libusb_bulk_transfer(lynsynHandle, inEndpoint, bytes, numBytes, &transfered, 100);
  if(ret != 0) {
    printf("LIBUSB ERROR: %s\n", libusb_error_name(ret));
  }

  assert(transfered == numBytes);

  return;
}

bool initLynsyn(void) {
  libusb_device *lynsynBoard;

  int r = libusb_init(&usbContext);

  if(r < 0) {
    printf("Init Error\n"); //there was an error
    return false;
  }
	  
  libusb_set_debug(usbContext, 3); //set verbosity level to 3, as suggested in the documentation

  bool found = false;
  int numDevices = libusb_get_device_list(usbContext, &devs);
  while(!found) {
    for(int i = 0; i < numDevices; i++) {
      struct libusb_device_descriptor desc;
      libusb_device *dev = devs[i];
      libusb_get_device_descriptor(dev, &desc);
      if(desc.idVendor == 0x10c4 && desc.idProduct == 0x8c1e) {
        printf("Found Lynsyn Device\n");
        lynsynBoard = dev;
        found = true;
        break;
      }
    }
    if(!found) {
      printf("Waiting for Lynsyn device\n");
      sleep(1);
      numDevices = libusb_get_device_list(usbContext, &devs);
    }
  }

  int err = libusb_open(lynsynBoard, &lynsynHandle);

  if(err < 0) {
    printf("Could not open USB device\n");
    return false;
  }

  if(libusb_kernel_driver_active(lynsynHandle, 0x1) == 1) {
    err = libusb_detach_kernel_driver(lynsynHandle, 0x1);
    if (err) {
      printf("Failed to detach kernel driver for USB. Someone stole the board?\n");
      return false;
    }
  }

  if((err = libusb_claim_interface(lynsynHandle, 0x1)) < 0) {
    printf("Could not claim interface 0x1, error number %d\n", err);
    return false;
  }

  struct libusb_config_descriptor * config;
  libusb_get_active_config_descriptor(lynsynBoard, &config);
  if(config == NULL) {
    printf("Could not retrieve active configuration for device :(\n");
    return false;
  }

  struct libusb_interface_descriptor interface = config->interface[1].altsetting[0];
  for(int ep = 0; ep < interface.bNumEndpoints; ++ep) {
    if(interface.endpoint[ep].bEndpointAddress & 0x80) {
      inEndpoint = interface.endpoint[ep].bEndpointAddress;
    } else {
      
      outEndpoint = interface.endpoint[ep].bEndpointAddress;
    }
  }

  return true;
}

void releaseLynsyn(void) {
  libusb_release_interface(lynsynHandle, 0x1);
  libusb_attach_kernel_driver(lynsynHandle, 0x1);
  libusb_free_device_list(devs, 1);

  libusb_close(lynsynHandle);
  libusb_exit(usbContext);
}

///////////////////////////////////////////////////////////////////////////////

bool programFpga(void) {
  int ret = system("program_flash -f lynsyn.mcs -flash_type s25fl128sxxxxxx0-spi-x1_x2_x4 -blank_check -verify -cable type xilinx_tcf");
  if(ret) {
    printf("Can't program the FPGA flash.  Possible problems:\n");
    printf("- Xilinx Vivado software problem\n");
    printf("- Lynsyn USB port is not connected to the PC\n");
    printf("- Xilinx USB cable is not connected to both PC and Lynsyn\n");
    printf("- Faulty soldering or components:\n");
    printf("  - U1 Flash\n");
    printf("  - R1, R2, R3\n");
    printf("  - U2 FPGA\n");
    printf("  - J5 FPGA_debug\n");
    return false;
  }

  printf("FPGA flashed OK\n");

  return true;
}

bool programMcu(void) {
  int ret = system("commander flash lynsyn.bin --device EFM32GG332F1024");

  if(ret) {
    printf("Can't program the MCU.  Possible problems:\n");
    printf("- SiLabs Simplicity Commander problem\n");
    printf("- Lynsyn USB port is not connected to the PC\n");
    printf("- EFM32 programmer is not connected to both PC and Lynsyn\n");
    printf("- Faulty soldering or components:\n");
    printf("  - U4 MCU\n");
    printf("  - J6 MCU_debug\n");
    return false;
  }

  printf("MCU flashed OK\n");
  return true;
}

///////////////////////////////////////////////////////////////////////////////

bool cleanNonVolatile(void) {
  // program flash with HW version info
  struct HwInitRequestPacket req;
  req.request.cmd = USB_CMD_HW_INIT;
#ifdef VERSION_20
  req.hwVersion = HW_VERSION_2_0;
#else
  req.hwVersion = HW_VERSION_2_1;
#endif
  sendBytes((uint8_t*)&req, sizeof(struct HwInitRequestPacket));

  // read back hw info
  struct RequestPacket initRequest;
  initRequest.cmd = USB_CMD_INIT;
  sendBytes((uint8_t*)&initRequest, sizeof(struct RequestPacket));

  struct InitReplyPacket initReply;
  getBytes((uint8_t*)&initReply, sizeof(struct InitReplyPacket));

#ifdef VERSION_20
  if(initReply.hwVersion != HW_VERSION_2_0) {
#else
  if(initReply.hwVersion != HW_VERSION_2_1) {
#endif
    printf("Non-volatile not OK\n");
    return false;
  }

  printf("Non-volatile memory cleared OK\n");
  return true;
}

///////////////////////////////////////////////////////////////////////////////

bool testUsb(void) {
  struct TestRequestPacket req;
  req.request.cmd = USB_CMD_TEST;
  req.testNum = TEST_USB;
  sendBytes((uint8_t*)&req, sizeof(struct TestRequestPacket));

  struct UsbTestReplyPacket reply;
  getBytes((uint8_t*)&reply, sizeof(struct UsbTestReplyPacket));

  for(int i = 0; i < 256; i++) {
    if(reply.buf[i] != i) {
      printf("Can't communicate over USB.  Possible problems:\n");
      printf("- Problems with libusb\n");
      printf("- Lynsyn USB port is not connected to the PC\n");
      printf("- Faulty soldering or components:\n");
      printf("  - U4 MCU\n");
      printf("  - J1 USB\n");
      printf("  - Y1 48MHz crystal\n");
      return false;
    }
  }

  printf("USB communication OK\n");

  return true;
}

bool testSpi(void) {
  struct TestRequestPacket req;
  req.request.cmd = USB_CMD_TEST;
  req.testNum = TEST_SPI;
  sendBytes((uint8_t*)&req, sizeof(struct TestRequestPacket));

  struct TestReplyPacket reply;
  getBytes((uint8_t*)&reply, sizeof(struct TestReplyPacket));

  if(reply.testStatus == FPGA_CONFIGURE_FAILED) {
    printf("Can't configure FPGA.  Possible problems:\n");
    printf("- Lynsyn USB cable was not replugged after FPGA flashing\n");
    printf("- Faulty soldering or components:\n");
    printf("  - U2 FPGA\n");
    printf("  - U4 MCU\n");
    return false;
  }

  if(reply.testStatus == FPGA_SPI_FAILED) {
    printf("Can't send SPI commands to FPGA.  Possible problems:\n");
    printf("- Lynsyn USB cable was not replugged after FPGA flashing\n");
    printf("- Incorrect bitfile\n");
    printf("- Faulty soldering or components:\n");
    printf("  - U2 FPGA\n");
    printf("  - U4 MCU\n");
    return false;
  }

  printf("SPI communication OK\n");
  return true;
}

bool testJtag(void) {
  struct TestRequestPacket req;
  req.request.cmd = USB_CMD_TEST;
  req.testNum = TEST_JTAG;
  sendBytes((uint8_t*)&req, sizeof(struct TestRequestPacket));

  struct TestReplyPacket reply;
  getBytes((uint8_t*)&reply, sizeof(struct TestReplyPacket));

  if(reply.testStatus) {
    printf("JTAG port test failed\n");
    printf("Failing pins: ");

    if(reply.testStatus & 1) printf("TMS "); 
    if(reply.testStatus & 2) printf("TDI ");
    if(reply.testStatus & 4) printf("TRST ");
    if(reply.testStatus & 8) printf("TCK ");
    if(reply.testStatus & 0x10) printf("TDO ");
    printf("\n");

    printf("Possible problems:\n");
    printf("- JTAG cable not connected between J3 and J4\n");
    printf("- Faulty soldering or components:\n");
    printf("  - U2 FPGA\n");
    printf("  - U16 U17 U18 level converters\n");
    printf("  - J3 JTAG_out\n");
    printf("  - J4 JTAG_in\n");
    printf("  - R37, R35, R36, R38, R39, R40\n");
    return false;
  }

  printf("JTAG ports OK\n");
  return true;
}

bool testOsc(void) {
  struct TestRequestPacket req;
  req.request.cmd = USB_CMD_TEST;
  req.testNum = TEST_OSC;
  sendBytes((uint8_t*)&req, sizeof(struct TestRequestPacket));

  struct TestReplyPacket reply;
  getBytes((uint8_t*)&reply, sizeof(struct TestReplyPacket));

  if(!reply.testStatus) {
    printf("FPGA oscillator not running.  Possible problems:\n");
    printf("- Faulty soldering or components:\n");
    printf("  - U2 FPGA\n");
    printf("  - U3 20MHz oscillator\n");
    return false;
  }

  printf("FPGA oscillator OK\n");
  return true;
}

void ledsOn(void) {
  struct TestRequestPacket req;
  req.request.cmd = USB_CMD_TEST;
  req.testNum = TEST_LEDS_ON;
  sendBytes((uint8_t*)&req, sizeof(struct TestRequestPacket));
  return;
}

void ledsOff(void) {
  struct TestRequestPacket req;
  req.request.cmd = USB_CMD_TEST;
  req.testNum = TEST_LEDS_OFF;
  sendBytes((uint8_t*)&req, sizeof(struct TestRequestPacket));
  return;
}

void adcCalibrate(uint8_t channel, double current, double maxCurrent, bool hw) {
  struct CalibrateRequestPacket req;
  req.request.cmd = USB_CMD_CAL;
  req.channel = channel;
  req.calVal = (current / maxCurrent) * 0xffd0;
  req.hw = hw;
  sendBytes((uint8_t*)&req, sizeof(struct CalibrateRequestPacket));
}

bool testAdc(unsigned channel, double val, double rl) {
  // get cal value
  struct RequestPacket initRequest;
  initRequest.cmd = USB_CMD_INIT;
  sendBytes((uint8_t*)&initRequest, sizeof(struct RequestPacket));
  struct InitReplyPacket initReply;
  getBytes((uint8_t*)&initReply, sizeof(struct InitReplyPacket));

  if((initReply.calibration[channel] < 0.8) || (initReply.calibration[channel] > 1.2)) {
    printf("Calibration error: Calibration value is %f\n", initReply.calibration[channel]);
    return false;
  }

  // get adc value
  struct TestRequestPacket req;
  req.request.cmd = USB_CMD_TEST;
  req.testNum = TEST_ADC;
  sendBytes((uint8_t*)&req, sizeof(struct TestRequestPacket));
  struct AdcTestReplyPacket reply;
  getBytes((uint8_t*)&reply, sizeof(struct AdcTestReplyPacket));

  // calculate current
#ifdef VERSION_20
  double v = (reply.current[channel] * ((double)LYNSYN_REF_VOLTAGE) / (double)LYNSYN_MAX_CURRENT_VALUE) * initReply.calibration[channel];
  double i = (1000 * v) / (LYNSYN_RS * rl);
#else
  double v = (reply.current[channel] * ((double)LYNSYN_REF_VOLTAGE) / (double)LYNSYN_MAX_CURRENT_VALUE) * initReply.calibration[channel];
  double vs = v / 20;
  double i = vs / rl;
#endif

  if((i < (val * 0.99)) || (i > (val * 1.01))) {
    printf("Calibration error: Current measured to %f, should be %f\n", i, val);
    return false;
  }

  return true;
}

void calibrateSensor(int sensor) {
  char shuntSize[80];
  char calCurrent[80];

  printf("*** Enter the size of the shunt resistor for sensor %d (default is %f):\n", sensor, rlDefault[sensor]);
  if(!fgets(shuntSize, 80, stdin)) {
    printf("I/O error\n");
    exit(-1);
  }

  double shuntSizeVal = 0;

  if(shuntSize[1] == 0) shuntSizeVal = rlDefault[sensor];
  else shuntSizeVal = strtod(shuntSize, NULL);

  double maxCurrentVal = 0.125/(double)shuntSizeVal;

  printf("%f ohm shunt resistor gives a maximum current of %fA\n", shuntSizeVal, maxCurrentVal);

  printf("*** Enter the value of the calibration current source connected to the sensor (should be slightly less than the max):\n");
  if(!fgets(calCurrent, 80, stdin)) {
    printf("I/O error\n");
    exit(-1);
  }

  double calCurrentVal = strtod(calCurrent, NULL);

  printf("Calibrating sensor %d with calibration current %f\n", sensor, calCurrentVal);

  adcCalibrate(sensor, calCurrentVal, maxCurrentVal, sensor == 0);
  if(!testAdc(sensor, calCurrentVal, shuntSizeVal)) exit(-1);
}

///////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[]) {
#ifdef VERSION_20
  printf("Compiled for Lynsyn V2.0\n\n");
#else
  printf("Compiled for Lynsyn V2.1\n\n");
#endif

  char choice[80] = "";

  while(choice[0] != '1' && choice[0] != '2') {
    printf("Which procedure do you want to perform?\n");
    printf("Enter '1' for complete programming, test and calibration.\n");
    printf("Enter '2' for only current sensor calibration\n");
    if(!fgets(choice, 80, stdin)) {
      printf("I/O error\n");
      exit(-1);
    }
  }

  printf("*** Connect Lynsyn to the PC USB port.\n");
  getchar();

  if(choice[0] == '1') {
#ifdef VERSION_20
    printf("*** Measure the voltage across C68.  Should be 1.0V\n");
    getchar();

    printf("*** Measure the voltage across C78.  Should be 1.8V\n");
    getchar();

    printf("*** Measure the voltage across C80.  Should be 3.3V\n");
    getchar();

    printf("*** Verify that LED D3 is lit.\n");
    getchar();

#else
    printf("*** Measure the voltage across C56.  Should be 1.0V\n");
    getchar();

    printf("*** Measure the voltage across C66.  Should be 1.8V\n");
    getchar();

    printf("*** Measure the voltage across C68.  Should be 3.3V\n");
    getchar();

    printf("*** Verify that LED D3 is lit.\n");
    getchar();
#endif

#ifdef VERSION_20
    printf("*** Connect a multimeter to upper side of R30.  Adjust RV1 until the voltage gets as close to 2.5V as possible\n");
    getchar();

#else
    printf("*** Connect a multimeter to TP1.  The negative side of C44 (the side closest to R20) can be used as ground reference.\nAdjust RV1 until the voltage gets as close to 2.5V as possible\n");
    getchar();
#endif

    printf("*** Connect the Xilinx USB cable to Lynsyn J5.\n");
    getchar();

    if(!programFpga()) exit(-1);

    printf("*** Remove Xilinx USB cable from lynsyn.\n");
    getchar();

    printf("*** Reboot Lynsyn by removing and replugging the USB cable.\n");
    getchar();

    printf("*** Connect EFM32 programmer to Lynsyn.\n");
    getchar();

#ifdef VERSION_20
    printf("*** Connect a cable between J2 and J3.\n");
    getchar();
#else
    printf("*** Connect a cable between J3 and J4.\n");
    getchar();
#endif

    if(!programMcu()) exit(-1);
    if(!initLynsyn()) exit(-1);
    if(!testUsb()) exit(-1);
    if(!cleanNonVolatile()) exit(-1);
    if(!testSpi()) exit(-1);
    if(!testJtag()) exit(-1);
    if(!testOsc()) exit(-1);

    ledsOn();

    printf("*** Verify that LEDs D1 and D2 are lit\n");
    getchar();

    ledsOff();

  } else {
    if(!initLynsyn()) exit(-1);
  }

#ifdef VERSION_20
  printf("Not doing sensor calibration for this HW version.\n");

#else

  for(int i = 0; i < 7; i++) {
    calibrateSensor(i);
  }

#endif

  printf("\nAll tests OK and all calibrations done\n");

  releaseLynsyn();
}

