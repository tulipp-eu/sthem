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

#include <argp.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <libusb.h>

#include "../mcu/common/usbprotocol.h"

//#define SAMPLE_PC
#define AVERAGE

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

double calculateCurrent(int16_t current, double offset, double gain, double rl) {
  double v;
  double vs;
  double i;

  v = (((double)current-offset) * ((double)LYNSYN_REF_VOLTAGE) / (double)LYNSYN_MAX_CURRENT_VALUE) * gain;

  if(hwVersion == HW_VERSION_2_0) {
    i = (1000 * v) / (LYNSYN_RS * rl);
  } else {
    vs = v / 20;
    i = vs / rl;
  }

  return i;
}

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

bool live(void) {

  return true;
}

///////////////////////////////////////////////////////////////////////////////

static char doc[] = "A live sampler for Lynsyn boards";
static char args_doc[] = "";

static struct argp_option options[] = {
  {"board", 'b', "version",   0, "Board version" },
  { 0 }
};

struct arguments {
  int board_version;
};

static error_t parse_opt (int key, char *arg, struct argp_state *state) {
  struct arguments *arguments = state->input;

  switch (key) {
    case 'b':
      arguments->board_version = strtol(arg, NULL, 10);
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

  argp_parse (&argp, argc, argv, 0, 0, &arguments);

  if(arguments.board_version < 1) {
    printf("Assuming board V2.1 or V2.2\n");
    arguments.board_version = 3;
  }

  if(!initLynsyn()) {
    printf("Can't init Lynsyn\n");
    exit(-1);
  }

  // get cal value
  struct RequestPacket initRequest;
  initRequest.cmd = USB_CMD_INIT;
  sendBytes((uint8_t*)&initRequest, sizeof(struct RequestPacket));
  struct InitReplyPacket initReply;
  getBytes((uint8_t*)&initReply, sizeof(struct InitReplyPacket));
  struct CalInfoPacket calInfo;
  getBytes((uint8_t*)&calInfo, sizeof(struct CalInfoPacket));

#ifdef SAMPLE_PC
  struct JtagInitRequestPacket req;
  req.request.cmd = USB_CMD_JTAG_INIT;

  memset(req.devices, 0, sizeof(req.devices));
  req.devices[0].idcode = 0x4ba00477;
  req.devices[0].irlen = 4;
  req.devices[1].idcode = 0x1372c093;
  req.devices[1].irlen = 6;
  req.devices[2].idcode = 0x5ba00477;
  req.devices[2].irlen = 4;
  req.devices[3].idcode = 0x14710093;
  req.devices[3].irlen = 12;
  req.devices[4].idcode = 0x04721093;
  req.devices[4].irlen = 12;
  req.devices[5].idcode = 0x28e20126;
  req.devices[5].irlen = 12;

  sendBytes((uint8_t*)&req, sizeof(struct JtagInitRequestPacket));
#endif

  while(true) {
    // send getsample packet

    struct GetSampleRequestPacket req;
    req.request.cmd = USB_CMD_GET_SAMPLE;
    req.flags = 0;

#ifdef SAMPLE_PC
    req.flags |= SAMPLING_FLAG_SAMPLE_PC;
#endif

#ifdef AVERAGE
    req.flags |= SAMPLING_FLAG_AVERAGE;
#endif

    sendBytes((uint8_t*)&req, sizeof(struct GetSampleRequestPacket));

    // receive sample reply

    struct SampleReplyPacket reply;
    getBytes((uint8_t*)&reply, sizeof(struct SampleReplyPacket));

    // print sample data

    printf("Flags %x --- ", reply.flags);

#ifdef SAMPLE_PC
    printf("PC sampled to ");
    for(int core = 0; core < MAX_CORES; core++) {
      printf("%016lx ", reply.pc[core]);
    }
    printf(" --- ");
#endif

    printf("Current measured to ");
    for(int sensor = 0; sensor < SENSORS; sensor++) {
      double rl = rlDefault[sensor];
      double i = calculateCurrent(reply.current[sensor], calInfo.offset[sensor], calInfo.gain[sensor], rl);
      printf("%f ", i);
    }

    printf("\n");

    sleep(1);
  }

  return 0;
}

