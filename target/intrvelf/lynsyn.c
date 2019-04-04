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
#include "lynsyn.h"

#define AVERAGE

///////////////////////////////////////////////////////////////////////////////

#define LYNSYN_MAX_CURRENT_VALUE 32768
#define LYNSYN_REF_VOLTAGE 2.5

#define LYNSYN_RS 8200
#define LSYNSYN_SENSORS 7

double rlDefault[LYNSYN_SENSORS] = {0.025, 0.05, 0.05, 0.1, 0.1, 1, 10};

///////////////////////////////////////////////////////////////////////////////

struct libusb_device_handle *lynsynHandle = NULL;
uint8_t outEndpoint;
uint8_t inEndpoint;
struct libusb_context *usbContext = NULL;
libusb_device **devs = NULL;

struct CalInfoPacket calInfo = {};
struct GetSampleRequestPacket req = {};
struct SampleReplyPacket reply = {};

#define USE_HW_VERSION HW_VERSION_2_2

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
      fprintf(stderr, "[LYNSYN] libusb error: %s\n", libusb_error_name(ret));
  }

  assert(transfered == numBytes);

  return;
}

bool initLynsyn(void) {
  libusb_device *lynsynBoard = NULL;

  if(libusb_init(&usbContext) < 0) {
      fprintf(stderr, "[LYNSYN] init Error\n"); //there was an error
      return false;
  }
	  
  libusb_set_debug(usbContext, 3); //set verbosity level to 3, as suggested in the documentation

  int numDevices = libusb_get_device_list(usbContext, &devs);
  for(int i = 0; i < numDevices; i++) {
      struct libusb_device_descriptor desc;
      libusb_device *dev = devs[i];
      libusb_get_device_descriptor(dev, &desc);
      if(desc.idVendor == 0x10c4 && desc.idProduct == 0x8c1e) {
          lynsynBoard = dev;
          break;
      }
  }
  if (lynsynBoard == NULL) {
      fprintf(stderr, "[LSYNSYN] Lynsyn device not found\n");
      return false;
  }


  int err = libusb_open(lynsynBoard, &lynsynHandle);

  if(err < 0) {
      fprintf(stderr, "[LSYNSYN] could not open USB device\n");
      return false;
  }

  if(libusb_kernel_driver_active(lynsynHandle, 0x1) == 1) {
      err = libusb_detach_kernel_driver(lynsynHandle, 0x1);
      if (err) {
          fprintf(stderr, "[LSYNSYN] failed to detach kernel driver for USB. Someone stole the board?\n");
          return false;
      }
  }

  if((err = libusb_claim_interface(lynsynHandle, 0x1)) < 0) {
      fprintf(stderr, "[LSYNSYN] could not claim interface 0x1, error number %d\n", err);
      return false;
  }

  struct libusb_config_descriptor * config;
  libusb_get_active_config_descriptor(lynsynBoard, &config);
  if(config == NULL) {
      fprintf(stderr, "[LYNSYN] could not retrieve active configuration for device :(\n");
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


  // get cal value
  struct RequestPacket initRequest;
  initRequest.cmd = USB_CMD_INIT;
  sendBytes((uint8_t*)&initRequest, sizeof(struct RequestPacket));
  struct InitReplyPacket initReply;
  getBytes((uint8_t*)&initReply, sizeof(struct InitReplyPacket));
  getBytes((uint8_t*)&calInfo, sizeof(struct CalInfoPacket));

  req.request.cmd = USB_CMD_GET_SAMPLE;
  req.flags = SAMPLING_FLAG_AVERAGE;

  return true;
}

void releaseLynsyn(void) {
  libusb_release_interface(lynsynHandle, 0x1);
  libusb_attach_kernel_driver(lynsynHandle, 0x1);
  libusb_free_device_list(devs, 1);

  libusb_close(lynsynHandle);
  libusb_exit(usbContext);
  lynsynHandle = NULL;
}



double adjustCurrent(int16_t const current, unsigned int const sensor) {
    double v = (((double)current - calInfo.offset[sensor]) * ((double)LYNSYN_REF_VOLTAGE) / (double)LYNSYN_MAX_CURRENT_VALUE) * calInfo.gain[sensor];
  
#if USE_HW_VERSION == HW_VERSION_2_0 
    return (1000 * v) / (LYNSYN_RS * rlDefault[sensor]);
#else
    return (v / 20) / rlDefault[sensor];
#endif
}

int16_t* retrieveCurrents() {
    if (lynsynHandle != NULL) {
        sendBytes((uint8_t*)&req, sizeof(struct GetSampleRequestPacket));
        getBytes((uint8_t*)&reply, sizeof(struct SampleReplyPacket));
    }
    return reply.current;
}
