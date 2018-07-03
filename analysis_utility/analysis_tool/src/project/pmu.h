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

#ifndef PMU_H
#define PMU_H

#include <libusb.h>

#include "analysis_tool.h"

#define LYNSYN_MAX_CORES 4
#define LYNSYN_SENSORS 7
#define LYNSYN_FREQ 48000000

class Measurement;

class Pmu {

private:
	struct libusb_device_handle *lynsynHandle;
  uint8_t outEndpoint;
  uint8_t inEndpoint;
	struct libusb_context *usbContext;
	libusb_device **devs;
  uint8_t swVersion;
  uint8_t hwVersion;
  double sensorCalibration[LYNSYN_SENSORS];

  struct SampleReplyPacket *curSample;
  uint8_t *endPtr;
  int64_t lastTime;

  void sendBytes(uint8_t *bytes, int numBytes);
  void getBytes(uint8_t *bytes, int numBytes);
  int getArray(uint8_t *bytes, int maxNum, int numBytes);

public:
  static const unsigned MAX_SENSORS = LYNSYN_SENSORS;
  static const unsigned MAX_CORES = LYNSYN_MAX_CORES;

  double rl[LYNSYN_SENSORS];
  double supplyVoltage[LYNSYN_SENSORS];

  Pmu() {}

  Pmu(double rl[LYNSYN_SENSORS], double supplyVoltage[LYNSYN_SENSORS]) {
    for(int i = 0; i < LYNSYN_SENSORS; i++) {
      this->rl[i] = rl[i];
      this->supplyVoltage[i] = supplyVoltage[i];
    }
  }

  bool init();
  void release();

  void collectSamples(uint8_t *buf, int bufSize, unsigned startCore, uint64_t startAddr, unsigned stopCore, uint64_t stopAddr);
  bool getNextSample(Measurement *m);

  unsigned numSensors() { return LYNSYN_SENSORS; }
  unsigned numCores() { return LYNSYN_MAX_CORES; }

  static double cyclesToSeconds(uint64_t cycles) { return cycles / (double)LYNSYN_FREQ; }
};

#endif

