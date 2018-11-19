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

#define SW_VERSION        SW_VERSION_1_2
#define SW_VERSION_STRING "V1.2"

#define HW_VERSION_2_0 0x20
#define HW_VERSION_2_1 0x21

#define SW_VERSION_1_0 1
#define SW_VERSION_1_1 0x11
#define SW_VERSION_1_2 0x12

#define USB_Package_length    72
#define USB_Data_length        64

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

#define USB_CMD_BootMode       'o'
#define USB_CMD_FlashErase     'x'
#define USB_CMD_FLASH_Save     'e'
#define USB_CMD_RESET          'r'

#define BP_TYPE_START 0
#define BP_TYPE_STOP  1

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

///////////////////////////////////////////////////////////////////////////////

//#define MAX_PACKET_SIZE (sizeof(struct BreakpointRequestPacket))
#define MAX_PACKET_SIZE (sizeof(struct FlashBootPackage))
#define MAX_SAMPLES 32

struct __attribute__((__packed__)) RequestPacket {
  uint8_t cmd;
};

struct __attribute__((__packed__)) StartSamplingRequestPacket {
  struct RequestPacket request;
  int64_t samplePeriod; // set to 0 for PC sampling, any other number for current sampling only
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
  ////change
  uint8_t hw;
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

struct __attribute__((__packed__)) SampleReplyPacket {
  int64_t time;
  uint64_t pc[4];
  int16_t current[7];
  int16_t : 16;
};

struct __attribute__((__packed__)) FlashBootPackage {
	struct RequestPacket request;
  uint16_t index;
  uint32_t flash_address;
  uint8_t end;
//  int8_t reserve[4];
  uint8_t Data[USB_Data_length];
};

struct __attribute__((__packed__)) BootInitReplyPackage {
	struct RequestPacket reply;
  uint16_t ready;
  uint16_t hwVersion;
  uint16_t swVersion;
 // uint16_t NANDflash_version;
  uint16_t BlVersion;
 // uint16_t boot_version2;
};

/*struct __attribute__((__packed__)) FlashResultPackage {
	struct RequestPacket reply;
  uint16_t result;
  uint32_t data_length;
  uint32_t flash_address;
};  */

/*struct __attribute__((__packed__)) ResetReplyPackage {
	struct RequestPacket reply;
  uint16_t result;
};  */

/*struct __attribute__((__packed__)) FlashErasePackage {
	struct RequestPacket request;
  uint32_t flash_address_start;
  uint32_t flash_address_end;
}; */

struct __attribute__((__packed__)) ResetPackage {
	struct RequestPacket request;
  uint8_t Reserve[3];
//  uint32_t version;
  uint32_t crc;
 // uint32_t dataLength;
 // uint32_t flashAddress;
 // uint32_t crc_header;
};

/*struct __attribute__((__packed__)) Flash_Struct {

  uint16_t version;
  uint16_t Reserve0;
  uint32_t Datalength;
  uint32_t DataCRC;
  uint32_t Reserve1;
  uint32_t Reserve2;
  uint32_t HeaderCRC;
}; */
//#define Flash_offset1 0x200000l
//#define Flash_offset2 0x400000l

//#define Flash_end1 0x3fffffl
//#define Flash_end2 0x5fffffl

#define Flash_Header 512
#define FLASH_BOOT_END                0x10000
#define FLASH_NEW_APPLICATION_START   0x80000
#define FLASH_NEW_APPLICATION_END     0xf0000
#define FLASH_SIZE                    0x100000
///////////////////////////////////////////////////////////////////////////////

#endif
