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
// Initialization, test and calibration program for Lynsyn V2.0, V2.1, V2.2
//
///////////////////////////////////////////////////////////////////////////////

#include <argp.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <libusb.h>

#include "../mcu/usbprotocol.h"

///////////////////////////////////////////////////////////////////////////////

#define LYNSYN_MAX_CURRENT_VALUE 32768
#define LYNSYN_REF_VOLTAGE 2.5

#define LYNSYN_RS 8200
double rlDefault[7] = {0.025, 0.05, 0.05, 0.1, 0.1, 1, 10};

///////////////////////////////////////////////////////////////////////////////

struct libusb_device_handle *lynsynHandle;
uint8_t outEndpoint;
uint8_t inEndpoint;
struct libusb_context *usbContext;
libusb_device **devs;
unsigned hwVersion = HW_VERSION_2_2;

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
  req.hwVersion = hwVersion;
  sendBytes((uint8_t*)&req, sizeof(struct HwInitRequestPacket));

  // read back hw info
  struct RequestPacket initRequest;
  initRequest.cmd = USB_CMD_INIT;
  sendBytes((uint8_t*)&initRequest, sizeof(struct RequestPacket));

  struct InitReplyPacket initReply;
  getBytes((uint8_t*)&initReply, sizeof(struct InitReplyPacket));

  if(initReply.hwVersion != hwVersion) {
    printf("Non-volatile not OK\n");
    return false;
  }

  printf("Non-volatile memory cleared OK\n");
  return true;
}

///////////////////////////////////////////////////////////////////////////////

bool testUsb(void) {
  for(int i = 0; i < 100; i++) {
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

void calSet(uint8_t channel, double val) {
  struct CalSetRequestPacket req;
  req.request.cmd = USB_CMD_CAL_SET;
  req.channel = channel;
  req.cal = val;
  sendBytes((uint8_t*)&req, sizeof(struct CalSetRequestPacket));
}

void adcSet(uint32_t val) {
  struct AdcSetRequestPacket req;
  req.request.cmd = USB_CMD_ADC_SET;
  req.cal = val;
  sendBytes((uint8_t*)&req, sizeof(struct AdcSetRequestPacket));
}

bool testAdc(unsigned channel, double val, double rl, double acceptance) {
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
  double v;
  double vs;
  double i;

  if(hwVersion == HW_VERSION_2_0) {
    v = (reply.current[channel] * ((double)LYNSYN_REF_VOLTAGE) / (double)LYNSYN_MAX_CURRENT_VALUE) * initReply.calibration[channel];
    i = (1000 * v) / (LYNSYN_RS * rl);
  } else {
    v = (reply.current[channel] * ((double)LYNSYN_REF_VOLTAGE) / (double)LYNSYN_MAX_CURRENT_VALUE) * initReply.calibration[channel];
    vs = v / 20;
    i = vs / rl;
  }

  printf("Current measured to %f, should be %f\n", i, val);

  if((i < (val * (1 - acceptance))) || (i > (val * (1 + acceptance)))) {
    printf("Calibration error: Current measured to %f, should be %f\n", i, val);
    return false;
  }

  return true;
}

void calibrateSensor(int sensor, double acceptance) {
  if((sensor < 0) || (sensor > 6)) {
    printf("Incorrect sensor number: %d\n", sensor+1);
    exit(-1);
  }

  if(hwVersion != HW_VERSION_2_0) {
    char shuntSize[80];
    char calCurrent[80];

    printf("*** Enter the size of the shunt resistor for sensor %d (default is %f):\n", sensor+1, rlDefault[sensor]);
    if(!fgets(shuntSize, 80, stdin)) {
      printf("I/O error\n");
      exit(-1);
    }

    double shuntSizeVal = 0;

    if(shuntSize[1] == 0) shuntSizeVal = rlDefault[sensor];
    else shuntSizeVal = strtod(shuntSize, NULL);

    double maxCurrentVal = 0.125/(double)shuntSizeVal;

    printf("%f ohm shunt resistor gives a maximum current of %fA\n\n", shuntSizeVal, maxCurrentVal);

    printf("*** Enter the exact value of the calibration current source connected to the sensor.\n"
           "This value should be slightly less than the max, %.4fA would be a good choice.\n"
           "Do not trust the current source display, use a multimeter to confirm:\n", maxCurrentVal*0.96);
    if(!fgets(calCurrent, 80, stdin)) {
      printf("I/O error\n");
      exit(-1);
    }

    double calCurrentVal = strtod(calCurrent, NULL);

    printf("Calibrating sensor %d with calibration current %f\n", sensor+1, calCurrentVal);

    adcCalibrate(sensor, calCurrentVal, maxCurrentVal, sensor == 0);
    if(!testAdc(sensor, calCurrentVal, shuntSizeVal, acceptance)) exit(-1);
  }
}

///////////////////////////////////////////////////////////////////////////////

void program(void) {
  printf("*** Connect the Xilinx USB cable to Lynsyn J5.\n");
  getchar();

  if(!programFpga()) exit(-1);

  printf("*** Remove Xilinx USB cable from lynsyn.\n");
  getchar();

  printf("*** Reboot Lynsyn by removing and replugging the USB cable.\n");
  getchar();

  printf("*** Connect EFM32 programmer to Lynsyn.\n");
  getchar();

  if(!programMcu()) exit(-1);
}

void automaticTests(bool withJtag) {
  if(!initLynsyn()) exit(-1);
  if(!testUsb()) exit(-1);
  if(!testSpi()) exit(-1);

  if(withJtag) {
    if(hwVersion == HW_VERSION_2_0) {
      printf("*** Connect a cable between J2 and J3.\n");
      getchar();
    } else {
      printf("*** Connect a cable between J3 and J4.\n");
      getchar();
    }

    if(!testJtag()) exit(-1);
  }

  if(!testOsc()) exit(-1);

  releaseLynsyn();

  printf("\nAll tests OK.\n");
}

void programTestAndCalibrate(double acceptance) {
  printf("This procedure programs, tests and calibrates the Lynsyn board.\n"
         "All lines starting with '***' requires you to do a certain action, and then press enter to continue.\n\n");

  printf("First step: Manual tests.\n\n");

  printf("*** Connect Lynsyn to the PC USB port.\n");
  getchar();

  if(hwVersion == HW_VERSION_2_0) {
    printf("*** Measure the voltage across C68.  Should be 1.0V.\n");
    getchar();

    printf("*** Measure the voltage across C78.  Should be 1.8V.\n");
    getchar();

    printf("*** Measure the voltage across C80.  Should be 3.3V.\n");
    getchar();

    printf("*** Verify that LED D3 is lit.\n");
    getchar();
  } else {
    printf("*** Measure the voltage across C56.  Should be 1.0V.\n");
    getchar();

    printf("*** Measure the voltage across C66.  Should be 1.8V.\n");
    getchar();

    printf("*** Measure the voltage across C68.  Should be 3.3V.\n");
    getchar();

    printf("*** Verify that LED D3 is lit.\n");
    getchar();
  }

  if(hwVersion == HW_VERSION_2_0) {
    printf("*** Connect a multimeter to upper side of R30.  Adjust RV1 until the voltage gets as close to 2.5V as possible.\n");
    getchar();
  } else if(hwVersion == HW_VERSION_2_1) {
    printf("*** Connect a multimeter to TP1.  The negative side of C44 (the side closest to R20) can be used as ground reference.\nAdjust RV1 until the voltage gets as close to 2.5V as possible.\n");
    getchar();
  } else {
    printf("*** Connect a multimeter to TP1.  Adjust RV1 until the voltage gets as close to 2.5V as possible.\n");
    getchar();
  }

  printf("*** Secure RV1 by applying a drop of nail polish on top.\n");
  getchar();

  printf("Second step: Programming.\n\n");

  program();

  printf("\nThird step: Automatic tests.\n\n");

  automaticTests(true);

  if(!initLynsyn()) exit(-1);

  printf("\nFourth step: Initializing non-volatile memory.\n\n");

  if(!cleanNonVolatile()) exit(-1);

  printf("\nFift step: More manual tests.\n\n");

  ledsOn();

  printf("*** Verify that LEDs D1 and D2 are lit.\n");
  getchar();

  ledsOff();

  printf("*** Verify that LEDs D1 and D2 are unlit.\n");
  getchar();

  printf("Sixt step: Sensor calibration.\n\n");

  if(hwVersion == HW_VERSION_2_0) {
    printf("Not doing sensor calibration for this HW version.\n");
  } else {
    for(int i = 0; i < 7; i++) {
      calibrateSensor(i, acceptance);
    }
  }

  releaseLynsyn();

  printf("\nAll tests OK and all calibrations done.\n");
}

void calSensor(double acceptance) {
  if(!initLynsyn()) exit(-1);

  if(hwVersion == HW_VERSION_2_0) {
    printf("Not doing sensor calibration for this HW version.\n");
  } else {
    char sensor[80] = "";

    while(sensor[0] != 'x') {
      printf("Which sensor do you want to calibrate ('x' for exit)?\n");
      if(!fgets(sensor, 80, stdin)) {
        printf("I/O error\n");
        exit(-1);
      }
      if((sensor[0] != 'x') && (sensor[0] != 'X'))  calibrateSensor(strtol(sensor, NULL, 10)-1, acceptance);
    }
  }

  releaseLynsyn();
}

void downloadCalData(void) {
  if(!initLynsyn()) exit(-1);

  printf("Enter filename to write to: ");
  char filename[80];
  int r = fscanf(stdin, "%s", filename);
  if(r != 1) {
    printf("I/O error\n");
    exit(-1);
  }

  FILE *fp = fopen(filename, "wb");

  if(!fp) {
    printf("Can't open file '%s'\n", filename);
    exit(-1);
  }

  struct RequestPacket initRequest;
  initRequest.cmd = USB_CMD_INIT;
  sendBytes((uint8_t*)&initRequest, sizeof(struct RequestPacket));

  struct InitReplyPacket initReply;
  getBytes((uint8_t*)&initReply, sizeof(struct InitReplyPacket));

  fwrite(initReply.calibration, 1, sizeof(initReply.calibration), fp);

  for(int i = 0; i < 7; i++) {
    printf("Calibration sensor %d: %f\n", i+1, initReply.calibration[i]);
  }

  fwrite(&initReply.adcCal, 1, sizeof(initReply.adcCal), fp);
  printf("ADC CAL: %x\n", (unsigned)initReply.adcCal);

  fclose(fp);

  releaseLynsyn();
}

void uploadCalData(void) {
  if(!initLynsyn()) exit(-1);

  printf("Enter filename to read from: ");
  char filename[80];
  int r = fscanf(stdin, "%s", filename);
  if(r != 1) {
    printf("I/O error\n");
    exit(-1);
  }

  FILE *fp = fopen(filename, "rb");

  if(!fp) {
    printf("Can't open file '%s'\n", filename);
    exit(-1);
  }

  double calibration[7];
  uint32_t adcCal;

  r = fread(calibration, 1, sizeof(calibration), fp);

  if(r != sizeof(calibration)) {
    printf("Can't read file: %d\n", r);
    exit(-1);
  }

  for(int i = 0; i < 7; i++) {
    printf("Calibration sensor %d: %f\n", i+1, calibration[i]);
    calSet(i, calibration[i]);
  }

  r = fread(&adcCal, 1, sizeof(adcCal), fp);

  if(r != sizeof(adcCal)) {
    printf("Can't read file: %d\n", r);
    exit(-1);
  }

  printf("ADC CAL: %x\n", adcCal);
  adcSet(adcCal);

  fclose(fp);

  releaseLynsyn();
}

///////////////////////////////////////////////////////////////////////////////

static char doc[] = "A test and calibration tool for Lynsyn boards";
static char args_doc[] = "";

static struct argp_option options[] = {
  {"board-version",  'b', "version",      0,  "Board version" },
  {"procedure",    'p', "procedure",      0,  "Which procedure to run" },
  {"acceptance",   'a', "value",      0, "Maximum allowed error in percentage (0.01 default" },
  { 0 }
};

struct arguments {
  int board_version;
  int procedure;
  double acceptance;
};

static error_t parse_opt (int key, char *arg, struct argp_state *state) {
  /* Get the input argument from argp_parse, which we
     know is a pointer to our arguments structure. */
  struct arguments *arguments = state->input;

  switch (key) {
    case 'b':
      arguments->board_version = strtol(arg, NULL, 10);
      break;
    case 'p':
      arguments->procedure = strtol(arg, NULL, 10);
      break;
    case 'a':
      arguments->acceptance = strtod(arg, NULL);
      break;

    case ARGP_KEY_ARG:
      if (state->arg_num >= 0)
        argp_usage (state);
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }

  return 0;
}

static struct argp argp = { options, parse_opt, args_doc, doc };

int main(int argc, char *argv[]) {
  struct arguments arguments;

  arguments.board_version = -1;
  arguments.procedure = -1;
  arguments.acceptance = 0.01;

  argp_parse (&argp, argc, argv, 0, 0, &arguments);

  if(arguments.board_version < 1) {
    char versionChoiceBuf[80];

    printf("Which HW version is the board?\n");
    printf("Enter '1' for V2.0\n");
    printf("Enter '2' for V2.1\n");
    printf("Enter '3' for V2.2\n");
    if(!fgets(versionChoiceBuf, 80, stdin)) {
      printf("I/O error\n");
      exit(-1);
    }

    arguments.board_version = strtol(versionChoiceBuf, NULL, 10);
  }
  if(arguments.procedure < 1) { 
    char choiceBuf[80];

    printf("Which procedure do you want to perform?\n");
    printf("Enter '1' for complete programming, test and calibration.\n");
    printf("Enter '2' for only current sensor calibration\n");
    printf("Enter '3' for only automatic testing\n");
    printf("Enter '4' for downloading cal data\n");
    printf("Enter '5' for uploading cal data\n");
    printf("Enter '6' for only MCU and FPGA flashing\n");
    if(!fgets(choiceBuf, 80, stdin)) {
      printf("I/O error\n");
      exit(-1);
    }

    arguments.procedure = strtol(choiceBuf, NULL, 10);
  }
    
  switch(arguments.board_version) {
    case 1: hwVersion = HW_VERSION_2_0; printf("Using HW V2.0\n"); break;
    case 2: hwVersion = HW_VERSION_2_1; printf("Using HW V2.1\n"); break;
    default:
    case 3: hwVersion = HW_VERSION_2_2; printf("Using HW V2.2\n"); break;
  }

  printf("\n");

  switch(arguments.procedure) {
    case 1:
      programTestAndCalibrate(arguments.acceptance);
      break;
    case 2:
      printf("*** Connect Lynsyn to the PC USB port.\n");
      getchar();

      calSensor(arguments.acceptance);
      break;
    case 3:
      printf("*** Connect Lynsyn to the PC USB port.\n");
      getchar();

      automaticTests(true);
      break;
    case 4:
      printf("*** Connect Lynsyn to the PC USB port.\n");
      getchar();

      downloadCalData();
      break;
    case 5:
      printf("*** Connect Lynsyn to the PC USB port.\n");
      getchar();

      uploadCalData();
      break;
    case 6:
      printf("*** Connect Lynsyn to the PC USB port.\n");
      getchar();

      program();
      automaticTests(false);
      break;
    default:
      return 0;
  }
}

