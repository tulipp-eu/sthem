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

#include <QtSql>

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

    printf("Lynsyn Hardware %x Firmware %x\n", hwVersion, swVersion);

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

  while((transfered == 0) && (ret == LIBUSB_ERROR_TIMEOUT)) {
    ret = libusb_bulk_transfer(lynsynHandle, inEndpoint, bytes, maxNum * numBytes, &transfered, 100);

    if((ret != 0) && (ret != LIBUSB_ERROR_TIMEOUT)) {
      printf("LIBUSB ERROR: %s\n", libusb_error_name(ret));
    }
  }

  assert((transfered % numBytes) == 0);
  assert(transfered);

  return transfered / numBytes;
}

void Pmu::storeRawSample(SampleReplyPacket *sample, int64_t timeSinceLast, double *minPower, double *maxPower, double *energy) {
  query.bindValue(":time", (qint64)sample->time);
  query.bindValue(":timeSinceLast", (qint64)timeSinceLast);
  query.bindValue(":pc1", (quint64)sample->pc[0]);
  query.bindValue(":pc2", (quint64)sample->pc[1]);
  query.bindValue(":pc3", (quint64)sample->pc[2]);
  query.bindValue(":pc4", (quint64)sample->pc[3]);

  double power[LYNSYN_SENSORS];

  for(int i = 0; i < LYNSYN_SENSORS; i++) {
    power[i] = currentToPower(i, sample->current[i]);
    if(power[i] < minPower[i]) minPower[i] = power[i];
    if(power[i] > maxPower[i]) maxPower[i] = power[i];
    energy[i] += power[i] * cyclesToSeconds(timeSinceLast);
  }

  query.bindValue(":power1", power[0]);
  query.bindValue(":power2", power[1]);
  query.bindValue(":power3", power[2]);
  query.bindValue(":power4", power[3]);
  query.bindValue(":power5", power[4]);
  query.bindValue(":power6", power[5]);
  query.bindValue(":power7", power[6]);

  bool success = query.exec();
  if((swVersion == SW_VERSION_1_1) && !success) {
    printf("Failed to insert %d\n", (unsigned)sample->time);
  } else {
    assert(success);
  }
}

void Pmu::collectSamples(bool useBp, bool samplePc, bool samplingModeGpio,
                         int64_t samplePeriod, unsigned startCore, uint64_t startAddr, unsigned stopCore, uint64_t stopAddr, 
                         uint64_t *samples, int64_t *minTime, int64_t *maxTime, double *minPower, double *maxPower,
                         double *runtime, double *energy) {
  {
    struct RequestPacket req;
    req.cmd = USB_CMD_JTAG_INIT;
    sendBytes((uint8_t*)&req, sizeof(struct RequestPacket));
  }

  if(useBp) {
    printf("Setting BP %lx on core %d and BP %lx on core %d\n", startAddr, startCore, stopAddr, stopCore);

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
  }

  if(swVersion <= SW_VERSION_1_1) {
    struct RequestPacket req;
    req.cmd = USB_CMD_START_SAMPLING;
    if(!samplePc) printf("Warning: PMU does not support measuring without PC sampling. Update firmware!\n");
    if(!useBp) {
      printf("PMU does not support measuring without breakpoints. Update firmware!\n");
      return;
    }
    sendBytes((uint8_t*)&req, sizeof(struct RequestPacket));

  } else if(swVersion == SW_VERSION_1_1) {
    struct StartSamplingRequestPacketV1_1 req;
    req.request.cmd = USB_CMD_START_SAMPLING;
    if(samplePc) {
      req.samplePeriod = 0;
    } else {
      req.samplePeriod = samplePeriod;
    }
    if(samplingModeGpio) {
      printf("Warning. PMU does not support measuring with GPIO control. Update firmware!\n");
    }
    if(!useBp) {
      printf("PMU does not support measuring without breakpoints. Update firmware!\n");
      return;
    }
    sendBytes((uint8_t*)&req, sizeof(struct StartSamplingRequestPacketV1_1));

  } else {
    struct StartSamplingRequestPacket req;
    req.request.cmd = USB_CMD_START_SAMPLING;
    req.samplePeriod = samplePeriod;
    req.flags = (samplePc ? SAMPLING_FLAG_SAMPLE_PC : 0) | (samplingModeGpio ? SAMPLING_FLAG_GPIO : 0) | (useBp ? SAMPLING_FLAG_BP : 0);
    sendBytes((uint8_t*)&req, sizeof(struct StartSamplingRequestPacket));
  }

  uint8_t *buf = (uint8_t*)malloc(MAX_SAMPLES * sizeof(SampleReplyPacket));

  *samples = 0;
  *minTime = 0;
  *maxTime = 0;
  for(int i = 0; i < LYNSYN_SENSORS; i++) {
    minPower[i] = INT_MAX;
    maxPower[i] = 0;
    energy[i] = 0;
  }

  int counter = 0;

  bool done = false;

  QSqlDatabase::database().transaction();

  query.prepare("INSERT INTO measurements (time, timeSinceLast, pc1, pc2, pc3, pc4, power1, power2, power3, power4, power5, power6, power7) "
                "VALUES (:time, :timeSinceLast, :pc1, :pc2, :pc3, :pc4, :power1, :power2, :power3, :power4, :power5, :power6, :power7)");

  int64_t lastTime = -1;

  while(!done) {
    counter++;
    if(swVersion <= SW_VERSION_1_1) {
      if((counter % 10000) == 0) printf("Got %ld samples...\n", *samples);
    } else {
      if((counter % 313) == 0) printf("Got %ld samples...\n", *samples);
    }

    int n = 1;

    if(swVersion <= SW_VERSION_1_1) {
      getBytes(buf, sizeof(struct SampleReplyPacketV1_0));
    } else {
      n = getArray(buf, MAX_SAMPLES, sizeof(struct SampleReplyPacket));
    }

    *samples += n;

    SampleReplyPacket *sample = (SampleReplyPacket*)buf;

    for(int i = 0; i < n; i++) {
      if(*minTime == 0) *minTime = sample->time;
      if(sample->time > *maxTime) *maxTime = sample->time;

      if(sample->time == -1) {
        printf("Sampling done\n");
        done = true;
        (*samples)--;
        break;

      } else {
        int64_t timeSinceLast = 0;
        if(lastTime != -1) timeSinceLast = sample->time - lastTime;
        lastTime = sample->time;

        storeRawSample(sample, timeSinceLast, minPower, maxPower, energy);
      }

      sample++;
    }
  }

  QSqlDatabase::database().commit();

  *runtime = cyclesToSeconds(*maxTime - *minTime);

  free(buf);
}

double Pmu::currentToPower(unsigned sensor, double current) {
  switch(hwVersion) {
    case HW_VERSION_2_0: {
      double vo = (current * (double)LYNSYN_REF_VOLTAGE) / (double)LYNSYN_MAX_CURRENT_VALUE;
      double i = (1000 * vo) / (LYNSYN_RS * rl[sensor]);
      return i * supplyVoltage[sensor];
    }
    case HW_VERSION_2_1: {
      double v = (current * ((double)LYNSYN_REF_VOLTAGE) / (double)LYNSYN_MAX_CURRENT_VALUE) * sensorCalibration[sensor];
      double vs = v / 20;
      double i = vs / rl[sensor];
      return i * supplyVoltage[sensor];
    }
  }
  return 0;
}
