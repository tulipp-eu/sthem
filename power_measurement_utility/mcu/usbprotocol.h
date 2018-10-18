/******************************************************************************
 *
 *  This file is part of the Lynsyn PMU firmware.
 *
 *  Copyright 2018 Asbjørn Djupdal, NTNU, TULIPP EU Project
 *
 *  Lynsyn is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Lynsyn is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Lynsyn.  If not, see <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

#ifndef USBPROTOCOL_H
#define USBPROTOCOL_H

#include <stdint.h>

#define SW_VERSION        SW_VERSION_1_3
#define SW_VERSION_STRING "V1.3"

#define HW_VERSION_2_0 0x20
#define HW_VERSION_2_1 0x21
#define HW_VERSION_2_2 0x22

#define SW_VERSION_1_0 1
#define SW_VERSION_1_1 0x11
#define SW_VERSION_1_2 0x12
#define SW_VERSION_1_3 0x13

///////////////////////////////////////////////////////////////////////////////

#define USB_CMD_INIT           'i'
#define USB_CMD_HW_INIT        'h'
#define USB_CMD_JTAG_INIT      'j'
#define USB_CMD_BREAKPOINT     'b'
#define USB_CMD_START_SAMPLING 's'
#define USB_CMD_CAL            'l'
#define USB_CMD_CAL_SET        'c'
#define USB_CMD_ADC_SET        'a'
#define USB_CMD_TEST           't'

#define BP_TYPE_START 0
#define BP_TYPE_STOP  1
#define BP_TYPE_FRAME 2

#define TEST_USB      0
#define TEST_SPI      1
#define TEST_JTAG     2
#define TEST_OSC      3
#define TEST_LEDS_ON  4
#define TEST_LEDS_OFF 5
#define TEST_ADC      6

#define FPGA_INIT_OK          0
#define FPGA_CONFIGURE_FAILED 1
#define FPGA_SPI_FAILED       2

#define SAMPLING_FLAG_SAMPLE_PC 1
#define SAMPLING_FLAG_BP        2
#define SAMPLING_FLAG_GPIO      4
#define SAMPLING_FLAG_PERIOD    8

#define SAMPLE_REPLY_FLAG_FRAME_DONE 1

///////////////////////////////////////////////////////////////////////////////

#define MAX_PACKET_SIZE (sizeof(struct BreakpointRequestPacket))
#define MAX_SAMPLES 32

struct __attribute__((__packed__)) RequestPacket {
  uint8_t cmd;
};

struct __attribute__((__packed__)) StartSamplingRequestPacketV1_1 {
  struct RequestPacket request;
  int64_t samplePeriod; // set to 0 for PC sampling, any other number for current sampling only
};

struct __attribute__((__packed__)) StartSamplingRequestPacket {
  struct RequestPacket request;
  int64_t samplePeriod; // set to 0 for PC sampling, any other number for current sampling only
  uint64_t flags;
};

struct __attribute__((__packed__)) BreakpointRequestPacket {
  struct RequestPacket request;
  uint8_t core;
  uint8_t bpType;
  uint64_t addr;
};

struct __attribute__((__packed__)) HwInitRequestPacket {
  struct RequestPacket request;
  uint8_t hwVersion;
};

struct __attribute__((__packed__)) CalibrateRequestPacket {
  struct RequestPacket request;
  uint8_t channel;
  int32_t calVal;
  bool hw;
};

struct __attribute__((__packed__)) CalSetRequestPacket {
  struct RequestPacket request;
  uint8_t channel;
  double cal;
};

struct __attribute__((__packed__)) AdcSetRequestPacket {
  struct RequestPacket request;
  uint32_t cal;
};

struct __attribute__((__packed__)) TestRequestPacket {
  struct RequestPacket request;
  uint8_t testNum;
};

///////////////////////////////////////////////////////////////////////////////

struct __attribute__((__packed__)) InitReplyPacket {
  uint8_t hwVersion;
  uint8_t swVersion;
  double calibration[7];
  uint32_t adcCal;
};

struct __attribute__((__packed__)) TestReplyPacket {
  uint32_t testStatus;
};

struct __attribute__((__packed__)) UsbTestReplyPacket {
  uint8_t buf[256];
};

struct __attribute__((__packed__)) AdcTestReplyPacket {
  int16_t current[7];
};

struct __attribute__((__packed__)) SampleReplyPacketV1_0 {
  int64_t time;
  uint64_t pc[4];
  int16_t current[7];
};

struct __attribute__((__packed__)) SampleReplyPacketV1_1 {
  int64_t time;
  uint64_t pc[4];
  int16_t current[7];
  int16_t : 16;
};

struct __attribute__((__packed__)) SampleReplyPacket {
  int64_t time;
  uint64_t pc[4];
  int16_t current[7];
  uint16_t flags;
};

///////////////////////////////////////////////////////////////////////////////

#endif
