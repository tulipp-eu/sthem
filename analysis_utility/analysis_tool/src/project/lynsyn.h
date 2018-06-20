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

#ifndef LYNSYN_H
#define LYNSYN_H

#include <libusb.h>

#include "analysis_tool.h"
#include "profile/measurement.h"

class Lynsyn {

private:
	struct libusb_device_handle *lynsynHandle;
  uint8_t outEndpoint;
  uint8_t inEndpoint;
	struct libusb_context *usbContext;
	libusb_device **devs;
  uint8_t hwVersion;
  double sensorCalibration[LYNSYN_SENSORS];

  struct SampleReplyPacket *curSample;
  uint8_t *endPtr;
  int64_t lastTime;

  void sendBytes(uint8_t *bytes, int numBytes);
  void getBytes(uint8_t *bytes, int numBytes);

public:
  double rl[LYNSYN_SENSORS];
  double supplyVoltage[LYNSYN_SENSORS];

  Lynsyn() {}

  Lynsyn(double rl[LYNSYN_SENSORS], double supplyVoltage[LYNSYN_SENSORS]) {
    for(int i = 0; i < LYNSYN_SENSORS; i++) {
      this->rl[i] = rl[i];
      this->supplyVoltage[i] = supplyVoltage[i];
    }
  }

  bool init();
  void release();

  void collectSamples(uint8_t *buf, int bufSize, unsigned startCore, uint64_t startAddr, unsigned stopCore, uint64_t stopAddr);
  bool getNextSample(Measurement *m);

};

#endif

