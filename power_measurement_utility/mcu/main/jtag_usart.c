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

#include "jtag.h"

#include <stdio.h>
#include <string.h>

#include <em_usart.h>

#ifndef USE_FPGA_JTAG_CONTROLLER

static inline void jtagPinWrite(bool clk, bool tdi, bool tms) {
  GPIO_PortOutSetVal(JTAG_PORT,
                     (clk << JTAG_TCK_BIT) | (tdi << JTAG_TDI_BIT) | (tms << JTAG_TMS_BIT), 
                     (1 << JTAG_TCK_BIT) | (1 << JTAG_TDI_BIT) | (1 << JTAG_TMS_BIT));
}

#ifdef USE_USART
static inline void usartOn(void) {
  jtagPinWrite(false, false, false);
  JTAG_USART->ROUTE = (JTAG_USART->ROUTE & ~_USART_ROUTE_LOCATION_MASK) | JTAG_USART_LOC
    | USART_ROUTE_TXPEN | USART_ROUTE_RXPEN | USART_ROUTE_CLKPEN;
}

static inline void usartOff(void) {
  JTAG_USART->ROUTE = 0;
}
#endif

static inline unsigned readBit(void) {
  return GPIO_PinInGet(JTAG_PORT, JTAG_TDO_BIT);
}

static inline void writeBit(bool tdi, bool tms) {
  jtagPinWrite(false, tdi, tms);
  jtagPinWrite(true, tdi, tms);
#ifdef DUMP_PINS
  printf("  TMS: %d TDI: %d TDO: %d\n", tms, tdi, readBit());
#endif
}

static inline uint8_t readWriteByte(uint8_t val, bool last) {
  uint8_t readValue = 0;

  for(int i = 0; i < 8; i++) {
    writeBit((val >> i) & 1, (i == 7) && last);
    readValue |= readBit() << i;
  }

  return readValue;
}

static inline uint32_t readWriteWord(uint32_t val, bool last) {
  uint32_t readValue = 0;

  for(int i = 0; i < 32; i++) {
    writeBit((val >> i) & 1, (i == 31) && last);
    readValue |= readBit() << i;
  }

  return readValue;
}

static void readWriteBuf(uint8_t *inBuf, uint8_t *outBuf, int size, bool last) {
  int bytes = size / 8;
  int leftovers = size % 8;

#ifdef USE_USART
  usartOn();

  if(!leftovers && last) {
    bytes--;
    leftovers = 8;
  }

  for(int i = 0; i < bytes; i++) {
    uint8_t txData = *inBuf++;

    while(!(JTAG_USART->STATUS & USART_STATUS_TXBL));
    JTAG_USART->TXDATA = (uint32_t)txData;
    while(!(JTAG_USART->STATUS & USART_STATUS_TXC));
    uint8_t val = (uint8_t)JTAG_USART->RXDATA;

#ifdef DUMP_PINS
    for(int i = 0; i < 8; i++) {
      printf("  TMS: %d TDI: %d TDO: %d\n", false, (txData >> i) & 1, (val >> i) & 1);
    }
#endif

    if(outBuf) {
      *outBuf++ = val;
    }
  }

  usartOff();
#else
  for(int i = 0; i < bytes; i++) {
    uint8_t val = readWriteByte(*inBuf++, (i == (bytes - 1)) && !leftovers && last);
    if(outBuf) *outBuf++ = val;
  }

#endif

  if(leftovers) {
    uint8_t val = 0;
    for(int i = 0; i < leftovers; i++) {
      writeBit((*inBuf >> i) & 1, (i == (leftovers-1)) && last);
      val |= readBit() << i;
    }
    if(outBuf) *outBuf = val;
  }
}

static inline void tmsSeq(uint32_t seq, int ticks) {
  while(ticks--) {
    writeBit(true, seq & 1);
    seq >>= 1;
  }
}

static inline void tdiSeq(uint32_t seq, int ticks) {
  while(ticks--) {
    writeBit(seq & 1, false);
    seq >>= 1;
  }
}

static inline void tdiSeqLast(uint32_t seq, int ticks) {
  while(ticks--) {
    if(ticks) writeBit(seq & 1, false);
    else writeBit(seq & 1, true);
    seq >>= 1;
  }
}

static inline void gotoShiftIr(void) {
#ifdef DUMP_PINS
  printf("Goto shift IR\n");
#endif
  tmsSeq(0x03, 4);
}

static inline void gotoShiftDr(void) {
#ifdef DUMP_PINS
  printf("Goto shift DR\n");
#endif
  tmsSeq(0x1, 3);
}

static inline void exitIrToShiftDr(void) {
#ifdef DUMP_PINS
  printf("Exit IR to shift DR\n");
#endif
  tmsSeq(0x3, 4);
}

static inline void exitIrToIdle(void) {
#ifdef DUMP_PINS
  printf("Exit IR to idle\n");
#endif
  tmsSeq(0x1, 2);
}

static inline void exitDrToIdle(void) {
#ifdef DUMP_PINS
  printf("Exit DR to idle\n");
#endif
  tmsSeq(0x1, 2);
}

void writeIr(uint32_t idcode, uint32_t ir) {
#ifdef DUMP_PINS
  printf("Write IR\n");
#endif
  static uint32_t lastIr = -1;

  if(ir != lastIr) {
    lastIr = ir;
    gotoShiftIr();
    for(int i = 0; i < numDevices; i++) {
      struct JtagDevice *dev = &devices[i];
      if(dev->idcode == idcode) {
#ifdef DUMP_PINS
        printf("Write IR %d\n", i);
#endif
        if(i == (numDevices - 1)) {
          tdiSeqLast(ir, dev->irlen);
        } else {
          tdiSeq(ir, dev->irlen);
        }
      } else {
#ifdef DUMP_PINS
        printf("Write ones %d\n", i);
#endif
        if(i == (numDevices - 1)) {
          tdiSeqLast(~0, dev->irlen);
        } else {
          tdiSeq(~0, dev->irlen);
        }
      }
    }
    exitIrToIdle();
  }
}

void readWriteDr(uint32_t idcode, uint8_t *din, uint8_t *dout, int size) {
#ifdef DUMP_PINS
  printf("Read/Write DR\n");
#endif
  int devNum;
  for(devNum = 0; devNum < numDevices; devNum++) {
    struct JtagDevice *dev = &devices[devNum];
    if(dev->idcode == idcode) {
      break;
    }
  }
  if(devNum == numDevices) panic("Can't find device");

  gotoShiftDr();
  
  unsigned postscan = numDevices - devNum - 1;

  // prescan
#ifdef DUMP_PINS
  printf("prescan\n");
#endif
  if(devNum) tdiSeq(~0, devNum);

  // scan
#ifdef DUMP_PINS
  printf("scan\n");
#endif
  readWriteBuf(din, dout, size, !postscan ? true : false);

  // postscan
#ifdef DUMP_PINS
  printf("postscan\n");
#endif
  if(postscan) tdiSeqLast(~0, postscan);

  exitDrToIdle();
}

int getNumDevices(void) {
#ifdef DUMP_PINS
  printf("Get Num Devices\n");
#endif
  uint8_t buf[MAX_JTAG_DEVICES];

  // set all devices in bypass by sending lots of 1s into the IR
  gotoShiftIr();
#ifdef DUMP_PINS
  printf("Flush IR with ones\n");
#endif
  memset(buf, 0xff, MAX_JTAG_DEVICES/8);
  readWriteBuf(buf, NULL, MAX_JTAG_DEVICES, true);

  // flush DR with 0
  exitIrToShiftDr();
#ifdef DUMP_PINS
  printf("Flush DR with zeros\n");
#endif
  memset(buf, 0, MAX_JTAG_DEVICES/8);
  readWriteBuf(buf, NULL, MAX_JTAG_DEVICES, false);

  // find numDevices by sending 1s until we get one back
  int num;

#ifdef DUMP_PINS
  printf("Send ones until we get a reply\n");
#endif

  for(num = 0; num < MAX_JTAG_DEVICES; num++) {
    writeBit(true, false);
    if(readBit()) break;
  }

  gotoResetThenIdle();

  return num;
}

void getIdCodes(uint32_t *idcodes) {
#ifdef DUMP_PINS
  printf("Get ID Codes\n");
#endif
  gotoShiftDr();
  for(int i = 0; i < numDevices; i++) {
#ifdef DUMP_PINS
    printf("Get ID code %d\n", i);
#endif
    *idcodes++ = readWriteWord(0, false);
  }
  gotoResetThenIdle();
}

uint32_t dpLowAccess(uint8_t RnW, uint16_t addr, uint32_t value) {
#ifdef DUMP_PINS
  printf("Low Access\n");
#endif
  bool APnDP = addr & ADIV5_APnDP;
  addr &= 0xff;
  uint8_t ack = JTAG_ACK_WAIT;

  uint8_t request_buf[8], response_buf[8];
  memset(request_buf, 0, 8);
  memset(response_buf, 0, 8);

  request_buf[4] = (value & 0xE0000000) >> 29;
  uint32_t tmp = (value << 3) | ((addr >> 1) & 0x06) | (RnW?1:0);
  request_buf[3] = (tmp & 0xFF000000) >> 24;
  request_buf[2] = (tmp & 0x00FF0000) >> 16;
  request_buf[1] = (tmp & 0x0000FF00) >> 8;
  request_buf[0] = (tmp & 0x000000FF);

  int tries = 1024;

  writeIr(dpIdcode, APnDP ? JTAG_IR_APACC : JTAG_IR_DPACC);

  while(tries > 0 && ack == JTAG_ACK_WAIT) {
    readWriteDr(dpIdcode, request_buf, response_buf, 35);
    ack = response_buf[0] & 0x07;
    tries--;
  }

  if(ack == JTAG_ACK_WAIT) {
    panic("Error: Invalid ACK (ack == JTAG_ACK_WAIT)\n");
  } else if(ack != JTAG_ACK_OK) {
    panic("Error: Invalid ACK (%x != JTAG_ACK_OK)\n", ack);
  }

  uint32_t response = 0x0;
  response = response_buf[0];
  response |= (((uint32_t)(response_buf[1])) << 8);
  response |= (((uint32_t)(response_buf[2])) << 16);
  response |= (((uint32_t)(response_buf[3])) << 24);
  response >>= 3;
  response |= (((uint32_t)(response_buf[4] & 0x07)) << 29);

  return response;
}

void gotoResetThenIdle(void) {
#ifdef DUMP_PINS
  printf("Goto Reset then Idle\n");
#endif
  tmsSeq(0x1f, 6);
}

void coreReadPcsrInit(void) {}

bool coreReadPcsrFast(uint64_t *pcs) {
  for(unsigned i = 0; i < numCores; i++) {
    pcs[i] = coreReadPcsr(&cores[i]);
  }

  if(zynqUltrascale) {
    return coreHalted(stopCore);
  } else {
    return pcs[0] == 0xffffffff;
  }
}

#endif
