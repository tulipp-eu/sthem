/******************************************************************************
 *
 *  This file is part of the TULIPP Analysis Utility
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

#include <assert.h>

#include <usbprotocol.h>

#define LYNSYN_MAX_CURRENT_VALUE 32768
#define LYNSYN_REF_VOLTAGE 2.5
#define LYNSYN_RS 8200

#include "pmu.h"
#include "profile/measurement.h"

bool Pmu::init() {
  libusb_device *lynsynBoard;

  int r = libusb_init(&usbContext);

  if(r < 0) {
    printf("Init Error\n");
    return false;
  }
	  
  libusb_set_debug(usbContext, 3);

  bool found = false;
  int numDevices = libusb_get_device_list(usbContext, &devs);
  for(int i = 0; i < numDevices; i++) {
    libusb_device_descriptor desc;
    libusb_device *dev = devs[i];
    libusb_get_device_descriptor(dev, &desc);
    if(desc.idVendor == 0x10c4 && desc.idProduct == 0x8c1e) {
      printf("Found Lynsyn Device\n");
      lynsynBoard = dev;
      found = true;
      break;
    }
  }

  if(!found) return false;

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

  {
    struct RequestPacket initRequest;
    initRequest.cmd = USB_CMD_INIT;
    sendBytes((uint8_t*)&initRequest, sizeof(struct RequestPacket));

    struct InitReplyPacket initReply;
    getBytes((uint8_t*)&initReply, sizeof(struct InitReplyPacket));

    if((initReply.swVersion != SW_VERSION_1_0) && (initReply.swVersion != SW_VERSION_1_1) && (initReply.swVersion != SW_VERSION_1_2)) {
      printf("Unsupported Lynsyn SW Version\n");
      return false;
    }

    if((initReply.hwVersion != HW_VERSION_2_0) && (initReply.hwVersion != HW_VERSION_2_1)) {
      printf("Unsupported Lynsyn HW Version: %x\n", initReply.hwVersion);
      return false;
    }

    swVersion = initReply.swVersion;
    hwVersion = initReply.hwVersion;

    for(unsigned i = 0; i < MAX_SENSORS; i++) {
      if((initReply.calibration[i] < 0.8) || (initReply.calibration[i] > 1.2)) {
        printf("Suspect calibration values\n");
        return false;
      }
      sensorCalibration[i] = initReply.calibration[i];
    }
  }

  return true;
}

void Pmu::release() {
  libusb_release_interface(lynsynHandle, 0x1);
  libusb_attach_kernel_driver(lynsynHandle, 0x1);
  libusb_free_device_list(devs, 1);

  libusb_close(lynsynHandle);
  libusb_exit(usbContext);
}

void Pmu::sendBytes(uint8_t *bytes, int numBytes) {
  int remaining = numBytes;
  int transfered = 0;
  while(remaining > 0) {
    libusb_bulk_transfer(lynsynHandle, outEndpoint, bytes, numBytes, &transfered, 0);
    remaining -= transfered;
    bytes += transfered;
  }
}

void Pmu::getBytes(uint8_t *bytes, int numBytes) {
  int transfered = 0;
  int ret = LIBUSB_ERROR_TIMEOUT;

  while(ret == LIBUSB_ERROR_TIMEOUT) {
    ret = libusb_bulk_transfer(lynsynHandle, inEndpoint, bytes, numBytes, &transfered, 100);
    if((ret != 0) && (ret != LIBUSB_ERROR_TIMEOUT)) {
      printf("LIBUSB ERROR: %s\n", libusb_error_name(ret));
    }
  }

  assert(transfered == numBytes);

  return;
}

int Pmu::getArray(uint8_t *bytes, int maxNum, int numBytes) {
  int transfered = 0;
  int ret = LIBUSB_ERROR_TIMEOUT;

  while(ret == LIBUSB_ERROR_TIMEOUT) {
    ret = libusb_bulk_transfer(lynsynHandle, inEndpoint, bytes, maxNum * numBytes, &transfered, 100);
    if((ret != 0) && (ret != LIBUSB_ERROR_TIMEOUT)) {
      printf("LIBUSB ERROR: %s\n", libusb_error_name(ret));
    }
  }

  assert((transfered % numBytes) == 0);
  assert(transfered);

  return transfered / numBytes;
}

void Pmu::collectSamples(uint8_t *buf, int bufSize, unsigned startCore, uint64_t startAddr, unsigned stopCore, uint64_t stopAddr) {
  {
    struct RequestPacket req;
    req.cmd = USB_CMD_JTAG_INIT;
    sendBytes((uint8_t*)&req, sizeof(struct RequestPacket));
  }

  {
    struct BreakpointRequestPacket req;
    req.request.cmd = USB_CMD_BREAKPOINT;
    req.core = startCore;
    req.bpType = BP_TYPE_START;
    req.addr = startAddr;

    sendBytes((uint8_t*)&req, sizeof(struct BreakpointRequestPacket));
  }

  {
    struct BreakpointRequestPacket req;
    req.request.cmd = USB_CMD_BREAKPOINT;
    req.core = stopCore;
    req.bpType = BP_TYPE_STOP;
    req.addr = stopAddr;

    sendBytes((uint8_t*)&req, sizeof(struct BreakpointRequestPacket));
  }

  {
    struct RequestPacket req;
    req.cmd = USB_CMD_START_SAMPLING;
    sendBytes((uint8_t*)&req, sizeof(struct RequestPacket));
  }

  int remainingSpace = bufSize;
  endPtr = buf;

  while(true) {
    if(swVersion <= SW_VERSION_1_1) {
      getBytes(endPtr, sizeof(struct SampleReplyPacketV1_0));

      endPtr += sizeof(struct SampleReplyPacket);
      remainingSpace -= sizeof(struct SampleReplyPacket);

    } else {
      int n = getArray(endPtr, MAX_SAMPLES, sizeof(struct SampleReplyPacket));

      endPtr += sizeof(struct SampleReplyPacket) * n;
      remainingSpace -= sizeof(struct SampleReplyPacket) * n;
    }

    if((unsigned)remainingSpace < sizeof(struct SampleReplyPacket)) {
      printf("Sample buffer full\n");
      break;
    }

    struct SampleReplyPacket *sample = (struct SampleReplyPacket *)(endPtr - sizeof(struct SampleReplyPacket));
    if(sample->time == -1) {
      printf("Sampling done\n");
      break;
    }
  }

  curSample = (struct SampleReplyPacket*)buf;
  lastTime = curSample->time;
}

bool Pmu::getNextSample(Measurement *m) {
  curSample++;

  if((uint8_t*)curSample >= endPtr) {
    return false;
  } 

  if(curSample->time == -1) {
    return false;
  }

  int64_t interval = curSample->time - lastTime;
  lastTime = curSample->time;

  for(unsigned core = 0; core < MAX_CORES; core++) {
    double p[MAX_SENSORS];

    for(unsigned sensor = 0; sensor < MAX_SENSORS; sensor++) {
      switch(hwVersion) {
        case HW_VERSION_2_0: {
          double vo = (curSample->current[sensor] * (double)LYNSYN_REF_VOLTAGE) / (double)LYNSYN_MAX_CURRENT_VALUE;
          double i = (1000 * vo) / (LYNSYN_RS * rl[sensor]);
          p[sensor] = i * supplyVoltage[sensor];
          break;
        }
        case HW_VERSION_2_1: {
          double v =
            (curSample->current[sensor] * ((double)LYNSYN_REF_VOLTAGE) / (double)LYNSYN_MAX_CURRENT_VALUE) * sensorCalibration[sensor];
          double vs = v / 20;
          double i = vs / rl[sensor];
          p[sensor] = i * supplyVoltage[sensor];
          break;
        }
      }
    }

    m[core] = Measurement(core, curSample->pc[core], curSample->time, interval, p);
  }

  return true;
}

