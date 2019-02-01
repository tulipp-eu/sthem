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
#include "fpga.h"

#include <stdio.h>
#include <string.h>

///////////////////////////////////////////////////////////////////////////////

static uint8_t recTdi[2048];
static uint8_t recTms[2048];
static uint8_t recRead[2048];
static unsigned recSize;

static uint8_t *tdiPtr;
static uint8_t *tmsPtr;
static uint8_t *readPtr;

///////////////////////////////////////////////////////////////////////////////

static inline uint32_t extractWord(unsigned pos, uint8_t *buf) {
  unsigned byte = pos / 8;
  unsigned bit = pos % 8;

  switch(bit) {
    case 0: return (buf[byte+3] << 24) | (buf[byte+2] << 16) | (buf[byte+1] << 8) | buf[byte];
    case 1: return ((buf[byte+4] & 0x01) << 31) | (buf[byte+3] << 23) | (buf[byte+2] << 15) | (buf[byte+1] << 7) | (buf[byte] >> 1);
    case 2: return ((buf[byte+4] & 0x03) << 30) | (buf[byte+3] << 22) | (buf[byte+2] << 14) | (buf[byte+1] << 6) | (buf[byte] >> 2);
    case 3: return ((buf[byte+4] & 0x07) << 29) | (buf[byte+3] << 21) | (buf[byte+2] << 13) | (buf[byte+1] << 5) | (buf[byte] >> 3);
    case 4: return ((buf[byte+4] & 0x0f) << 28) | (buf[byte+3] << 20) | (buf[byte+2] << 12) | (buf[byte+1] << 4) | (buf[byte] >> 4);
    case 5: return ((buf[byte+4] & 0x1f) << 27) | (buf[byte+3] << 19) | (buf[byte+2] << 11) | (buf[byte+1] << 3) | (buf[byte] >> 5);
    case 6: return ((buf[byte+4] & 0x3f) << 26) | (buf[byte+3] << 18) | (buf[byte+2] << 10) | (buf[byte+1] << 2) | (buf[byte] >> 6);
    case 7: return ((buf[byte+4] & 0x7f) << 25) | (buf[byte+3] << 17) | (buf[byte+2] <<  9) | (buf[byte+1] << 1) | (buf[byte] >> 7);
  }

  return 0;
}

static inline uint8_t extractAck(unsigned pos, uint8_t *buf) {
  unsigned byte = pos / 8;
  unsigned bit = pos % 8;

  switch(bit) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5: return (buf[byte] >> bit) & 0x7;
    case 6: return ((buf[byte+1] & 0x1) << 2) | ((buf[byte] >> 6) & 0x3);
    case 7: return ((buf[byte+1] & 0x3) << 1) | ((buf[byte] >> 7) & 0x1);
  }

  return 0;
}

///////////////////////////////////////////////////////////////////////////////

#ifdef DUMP_PINS
static void dumpRec(void) {
  unsigned bytes = recSize / 8;
  if(recSize % 8) bytes++;

  unsigned n = recSize;

  for(int i = 0; i < bytes; i++) {
    for(int j = 0; j < 8 && n; j++) {
      printf("TMS: %d TDI: %d Read: %d\n", (recTms[i] >> j) & 1, (recTdi[i] >> j) & 1, (recRead[i] >> j) & 1);
      n--;
    }
  }
}
#endif

static void startRec(void) {
  recSize = 0;
  tdiPtr = recTdi;
  tmsPtr = recTms;
  readPtr = recRead;
  *tdiPtr = 0;
  *tmsPtr = 0;
  *readPtr = 0;
}

static void sendRec(void) {
#ifdef DUMP_PINS
  dumpRec();
#endif
  writeSeq(recSize, recTdi, recTms);
}

static void storeRec(void) {
#ifdef DUMP_PINS
  dumpRec();
#endif
  storeSeq(recSize, recTdi, recTms, recRead);
}

static void readWriteRec(uint8_t *response_buf) {
#ifdef DUMP_PINS
  dumpRec();
#endif
  readWriteSeq(recSize, recTdi, recTms, response_buf);
}

static void recordSeq(unsigned size, uint8_t *tdiData, uint8_t *tmsData, uint8_t *readData) {
  for(int i = 0; i < size; i++) {
    unsigned sourceBit = i % 8;
    unsigned targetBit = recSize % 8;

    if(*tdiData & (1 << sourceBit)) *tdiPtr |= (1 << targetBit);
    if(*tmsData & (1 << sourceBit)) *tmsPtr |= (1 << targetBit);
    if(readData) {
      if(*readData & (1 << sourceBit)) *readPtr |= (1 << targetBit);
    }

    if(sourceBit == 7) {
      tdiData++;
      tmsData++;
      if(readData) readData++;
    }
    if(targetBit == 7) {
      tdiPtr++;
      tmsPtr++;
      readPtr++;
      *tdiPtr = 0;
      *tmsPtr = 0;
      *readPtr = 0;
    }

    recSize++;
  }
}

///////////////////////////////////////////////////////////////////////////////

static inline void gotoShiftIr(void) {
  uint8_t tms = 0x03;
  uint8_t tdi = 0;

#ifdef DUMP_PINS
  printf("Goto shift IR\n");
#endif

  recordSeq(4, &tdi, &tms, NULL);

#ifdef DUMP_PINS
  sendRec();
  startRec();
#endif
}

static inline void exitIrToShiftDr(void) {
  uint8_t tms = 0x03;
  uint8_t tdi = 0;

  recordSeq(4, &tdi, &tms, NULL);

#ifdef DUMP_PINS
  printf("Exit IR to shift DR\n");
  sendRec();
  startRec();
#endif
}

static inline void gotoShiftDr(void) {
  uint8_t tms = 0x01;
  uint8_t tdi = 0;

  recordSeq(3, &tdi, &tms, NULL);

#ifdef DUMP_PINS
  printf("Goto shift DR\n");
  sendRec();
  startRec();
#endif
}

static inline void exitIrToIdle(void) {
  uint8_t tms = 0x01;
  uint8_t tdi = 0;

  recordSeq(2, &tdi, &tms, NULL);

#ifdef DUMP_PINS
  printf("Exit IR to idle\n");
  sendRec();
  startRec();
#endif
}

static inline void exitDrToIdle(void) {
  uint8_t tms = 0x01;
  uint8_t tdi = 0;

  recordSeq(2, &tdi, &tms, NULL);

#ifdef DUMP_PINS
  printf("Exit DR to idle\n");
  sendRec();
  startRec();
#endif
}

static void writeIrInt(uint32_t idcode, uint32_t ir) {
  static uint32_t lastIr = -1;

  if(ir != lastIr) {
    lastIr = ir;
    gotoShiftIr();
    for(int i = 0; i < numDevices; i++) {
      struct JtagDevice *dev = &devices[i];
      if(dev->idcode == idcode) {
        uint8_t tms = 0;
        uint8_t tdi = ir;

        if(i == (numDevices - 1)) {
          tms = 1 << (dev->irlen - 1);
        }

        recordSeq(dev->irlen, &tdi, &tms, NULL);

#ifdef DUMP_PINS
        printf("Write IR %d\n", i);
        sendRec();
        startRec();
#endif
      } else {
        uint8_t tms[4] = {0, 0, 0, 0};
        uint8_t tdi[4] = {~0, ~0, ~0, ~0};

        if(i == (numDevices - 1)) {
          // set tms end bit
          int byte = (dev->irlen-1) / 8;
          int bit = (dev->irlen-1) % 8;
          tms[byte] = 1 << bit;
        }

        recordSeq(dev->irlen, tdi, tms, NULL);

#ifdef DUMP_PINS
        printf("Write ones %d\n", i);
        sendRec();
        startRec();
#endif
      }
    }
    exitIrToIdle();
  }
}

static unsigned readWriteDrPre(uint32_t idcode) {
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

  if(devNum) {
    uint8_t tms[MAX_JTAG_DEVICES/8];
    uint8_t tdi[MAX_JTAG_DEVICES/8];

    memset(tms, 0, MAX_JTAG_DEVICES/8);
    memset(tdi, ~0, MAX_JTAG_DEVICES/8);

    recordSeq(devNum, tdi, tms, NULL);

#ifdef DUMP_PINS
    printf("prescan\n");
    sendRec();
    startRec();
#endif
  }

  return postscan;
}

static void readWriteDrPost(unsigned postscan) {
  // postscan
  if(postscan) {
    uint8_t tms[MAX_JTAG_DEVICES/8];
    uint8_t tdi[MAX_JTAG_DEVICES/8];

    memset(tms, 0, MAX_JTAG_DEVICES/8);
    memset(tdi, ~0, MAX_JTAG_DEVICES/8);

    int byte = (postscan-1) / 8;
    int bit = (postscan-1) % 8;
    tms[byte] = 1 << bit;

    recordSeq(postscan, tdi, tms, NULL);

#ifdef DUMP_PINS
    printf("postscan\n");
    sendRec();
    startRec();
#endif
  }

  exitDrToIdle();
}

static void dpLowAccessFast(uint8_t RnW, uint16_t addr, uint32_t value, bool discard) {
  bool APnDP = addr & ADIV5_APnDP;
  addr &= 0xff;

  int readBits = 3;
  if(RnW) readBits += 32;

  writeIrInt(dpIdcode, APnDP ? JTAG_IR_APACC : JTAG_IR_DPACC);

  unsigned postscan = readWriteDrPre(dpIdcode);

  { // addr + readbit
    uint8_t tdi = ((addr >> 1) & 0x06) | (RnW?1:0);
    uint8_t tms = 0;
    uint8_t read = 0x07;
    recordSeq(3, &tdi, &tms, &read);
  }

  { // value
    uint8_t tms[4];
    uint8_t read[4];
    memset(tms, 0, 4);
    if(!discard) memset(read, 0xff, 4);
    else memset(read, 0, 4);

    if(!postscan) {
      tms[3] = 0x80;
    }
    recordSeq(32, (uint8_t*)&value, tms, read);
  }

  readWriteDrPost(postscan);
}

static void dpReadFast(uint16_t addr) {
  dpLowAccessFast(ADIV5_LOW_READ, addr, 0, true);
  dpLowAccessFast(ADIV5_LOW_READ, ADIV5_DP_RDBUFF, 0, false);
}

static void dpWriteFast(uint16_t addr, uint32_t value) {
  dpLowAccessFast(ADIV5_LOW_WRITE, addr, value, true);
}

static void apWriteFast(uint16_t addr, uint32_t value) {
  dpWriteFast(ADIV5_DP_SELECT, ((uint32_t)apSel << 24)|(addr & 0xF0));
  dpWriteFast(addr, value);
}

static void coreReadRegFast(struct Core core, uint16_t reg) {
  uint32_t addr = core.baddr + 4*reg;
  apWriteFast(ADIV5_AP_TAR, addr);
  dpReadFast(ADIV5_AP_DRW);
}

///////////////////////////////////////////////////////////////////////////////

void writeIr(uint32_t idcode, uint32_t ir) {
#ifdef DUMP_PINS
  printf("Write IR %x\n", (unsigned)ir);
#endif
  startRec();
  writeIrInt(idcode, ir);
  sendRec();
}

void readWriteDr(uint32_t idcode, uint8_t *din, uint8_t *dout, int size) {
#ifdef DUMP_PINS
  printf("Read/Write DR\n");
#endif
  startRec();
  int postscan = readWriteDrPre(idcode);
  sendRec();

  startRec();

#ifdef DUMP_PINS
  printf("scan\n");
#endif

  uint8_t tms[MAX_SEQ_SIZE];
  assert(size <= MAX_SEQ_SIZE);
  memset(tms, 0, size);

  if(!postscan) {
#ifdef DUMP_PINS
    printf("no postscan\n");
#endif
    int byte = (size-1) / 8;
    int bit = (size-1) % 8;
    tms[byte] = 1 << bit;
  }

  recordSeq(size, din, tms, NULL);
  if(dout) readWriteRec(dout);
  else sendRec();
  
  startRec();
  readWriteDrPost(postscan);
  sendRec();
}

uint32_t dpLowAccess(uint8_t RnW, uint16_t addr, uint32_t value) {
#ifdef DUMP_PINS
  printf("Low Access\n");
#endif

  startRec();

  bool APnDP = addr & ADIV5_APnDP;
  addr &= 0xff;
  uint8_t ack = JTAG_ACK_WAIT;

  uint8_t response_buf[4];
  int tries = 1024;

  writeIrInt(dpIdcode, APnDP ? JTAG_IR_APACC : JTAG_IR_DPACC);

  sendRec();

  while(tries > 0 && ack == JTAG_ACK_WAIT) {
    startRec();
    unsigned postscan = readWriteDrPre(dpIdcode);
    sendRec();

    { // addr + readbit
      startRec();

      uint8_t tdi = ((addr >> 1) & 0x06) | (RnW?1:0);
      uint8_t tms = 0;
      uint8_t read = 0x07;
      recordSeq(3, &tdi, &tms, &read);

      readWriteRec(&ack);
    }

    { // value
      startRec();

      uint8_t tms[4];
      uint8_t read[4];
      memset(tms, 0, 4);
      if(RnW) memset(read, 0xff, 4);
      else memset(read, 0, 4);

      if(!postscan) {
        tms[3] = 0x80;
      }
      recordSeq(32, (uint8_t*)&value, tms, read);

      if(RnW) readWriteRec(response_buf);
      else sendRec();
    }

    startRec();
    readWriteDrPost(postscan);
    sendRec();

    tries--;
  }

  if(ack == JTAG_ACK_WAIT) {
    panic("Error: Invalid ACK (ack == JTAG_ACK_WAIT)\n");
  } else if(ack != JTAG_ACK_OK) {
    panic("Error: Invalid ACK (%x != JTAG_ACK_OK)\n", ack);
  }

  return extractWord(0, response_buf);
}

int getNumDevices(void) {
#ifdef DUMP_PINS
  printf("Get Num Devices\n");
#endif

  uint8_t tdi[MAX_JTAG_DEVICES/8];
  uint8_t tms[MAX_JTAG_DEVICES/8];

  startRec();

  // set all devices in bypass by sending lots of 1s into the IR
  gotoShiftIr();
  memset(tdi, 0xff, MAX_JTAG_DEVICES/8);
  memset(tms, 0, MAX_JTAG_DEVICES/8);
  tms[(MAX_JTAG_DEVICES/8)-1] = 0x80;
  recordSeq(MAX_JTAG_DEVICES, tdi, tms, NULL);

#ifdef DUMP_PINS
  printf("send ones\n");
  sendRec();
  startRec();
#endif

  // flush DR with 0
  exitIrToShiftDr();
  memset(tdi, 0, MAX_JTAG_DEVICES/8);
  memset(tms, 0, MAX_JTAG_DEVICES/8);
  recordSeq(MAX_JTAG_DEVICES, tdi, tms, NULL);

#ifdef DUMP_PINS
  printf("flush with 0\n");
#endif

  sendRec();

  // find numDevices by sending 1s until we get one back
  int num;

  for(num = 0; num < MAX_JTAG_DEVICES; num++) {
    uint8_t tms = 0;
    uint8_t tdi = 0x01;
    uint8_t tdo;

#ifdef DUMP_PINS
    printf("test with 1\n");
    printf("TMS: %d TDI: %d Read: %d\n", 0, 1, 1);
#endif

    readWriteSeq(1, &tdi, &tms, &tdo);
    if(tdo & 1) break;
  }

  gotoResetThenIdle();

  return num;
}

void getIdCodes(uint32_t *idcodes) {
#ifdef DUMP_PINS
  printf("Get ID Codes\n");
#endif

  startRec();
  gotoShiftDr();
  sendRec();

  for(int i = 0; i < numDevices; i++) {
    uint8_t tms[4] = {0, 0, 0, 0};
    uint8_t tdi[4] = {0, 0, 0, 0};
    uint8_t tdo[4];

#ifdef DUMP_PINS
    printf("read id code\n");
    for(int i = 0; i < 32; i++) {
      printf("TMS: %d TDI: %d Read: %d\n", 0, 0, 1);
    }
#endif

    readWriteSeq(32, tdi, tms, tdo);

    *idcodes++ = extractWord(0, tdo);

    printf("Got %x\n", (unsigned)extractWord(0, tdo));
  }

  gotoResetThenIdle();
}

void gotoResetThenIdle(void) {
  uint8_t tms = 0x1f;
  uint8_t tdi = 0;

  writeSeq(6, &tdi, &tms);
}

void coreReadPcsrInit(void) {
  startRec();

  if(zynqUltrascale) {
    for(int i = 0; i < numCores; i++) {
      if(cores[i].enabled) {
        coreReadRegFast(cores[i], A53_PCSR_H);
        coreReadRegFast(cores[i], A53_PCSR_L);
      }
    }
    coreReadRegFast(cores[stopCore], A53_PRSR);
  } else {
    for(int i = 0; i < numCores; i++) {
      coreReadRegFast(cores[i], A9_PCSR);
    }
  }

  storeRec();
}

bool coreReadPcsrFast(uint64_t *pcs) {
  bool halted;
  uint8_t *buf;

  executeSeq();

  if(zynqUltrascale) {
    buf = readSeq(numEnabledCores*88 + 44);
    
    unsigned core = 0;

    for(unsigned i = 0; i < numCores; i++) {
      if(cores[i].enabled) {
        uint8_t ack0 = extractAck(core*88 + 0, buf);
        uint8_t ack1 = extractAck(core*88 + 3, buf);
        uint8_t ack2 = extractAck(core*88 + 6, buf);
        uint8_t ack3 = extractAck(core*88 + 9, buf);

        uint8_t ack4 = extractAck(core*88 + 44, buf);
        uint8_t ack5 = extractAck(core*88 + 47, buf);
        uint8_t ack6 = extractAck(core*88 + 50, buf);
        uint8_t ack7 = extractAck(core*88 + 53, buf);

        if((ack0 != JTAG_ACK_OK) || (ack1 != JTAG_ACK_OK) || (ack2 != JTAG_ACK_OK) || (ack3 != JTAG_ACK_OK) ||
           (ack4 != JTAG_ACK_OK) || (ack5 != JTAG_ACK_OK) || (ack6 != JTAG_ACK_OK) || (ack7 != JTAG_ACK_OK)) {
          panic("Invalid ACK %x %x %x %x %x %x %x %x\n", ack0, ack1, ack2, ack3, ack4, ack5, ack6, ack7);
        }

        uint32_t dbgpcsrHigh = extractWord(core*88 + 12, buf);
        uint32_t dbgpcsrLow = extractWord(core*88 + 56, buf);

        uint64_t dbgpcsr = ((uint64_t)dbgpcsrHigh << 32) | dbgpcsrLow;

        if(dbgpcsr == 0xffffffff) {
          pcs[i] = 0;
        } else {
          pcs[i] = calcOffset(dbgpcsr);
        }

        core++;

      } else {
        pcs[i] = 0;
      }
    }

    uint8_t ack0 = extractAck(numEnabledCores*88 + 0, buf);
    uint8_t ack1 = extractAck(numEnabledCores*88 + 3, buf);
    uint8_t ack2 = extractAck(numEnabledCores*88 + 6, buf);
    uint8_t ack3 = extractAck(numEnabledCores*88 + 9, buf);

    if((ack0 != JTAG_ACK_OK) || (ack1 != JTAG_ACK_OK) || (ack2 != JTAG_ACK_OK) || (ack3 != JTAG_ACK_OK)) {
      panic("Invalid ACK %x %x %x %x\n", ack0, ack1, ack2, ack3);
    }

    uint32_t prsr = extractWord(numEnabledCores*88 + 12, buf);
    halted = prsr & (1 << 4);

  } else {
    buf = readSeq(numCores*44);

    for(unsigned i = 0; i < numCores; i++) {
      uint8_t ack0 = extractAck(i*44 + 0, buf);
      uint8_t ack1 = extractAck(i*44 + 3, buf);
      uint8_t ack2 = extractAck(i*44 + 6, buf);
      uint8_t ack3 = extractAck(i*44 + 9, buf);

      if((ack0 != JTAG_ACK_OK) || (ack1 != JTAG_ACK_OK) || (ack2 != JTAG_ACK_OK) || (ack3 != JTAG_ACK_OK)) {
        panic("Invalid ACK %x %x %x %x\n", ack0, ack1, ack2, ack3);
      }

      uint32_t dbgpcsr = extractWord(i*44 + 12, buf);

      if(dbgpcsr == 0xffffffff) {
        pcs[i] = dbgpcsr;
      } else {
        pcs[i] = calcOffset(dbgpcsr);
      }
    }

    if(pcs[0] == 0xffffffff) halted = true;
    else halted = false;
  }

  return halted;
}
