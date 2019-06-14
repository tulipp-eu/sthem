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

bool dumpPins = false;

static uint8_t recTdi[2048];
static uint8_t recTms[2048];
static uint8_t recRead[2048];
static unsigned seqStart;
static unsigned seqSize;

static uint8_t recReadFlag[64];
static uint16_t recInitPos[64];
static uint16_t recLoopPos[64];
static uint16_t recAckPos[64];
static uint16_t recEndPos[64];
static unsigned progSize;

static uint8_t *tdiPtr;
static uint8_t *tmsPtr;
static uint8_t *readPtr;

static uint8_t *readFlagPtr;
static uint16_t *initPosPtr;
static uint16_t *loopPosPtr;
static uint16_t *ackPosPtr;
static uint16_t *endPosPtr;

static unsigned fastBitsToRead;

///////////////////////////////////////////////////////////////////////////////

uint32_t extractWord(unsigned *pos, uint8_t *buf) {
  unsigned byte = *pos / 8;
  unsigned bit = *pos % 8;

  *pos += 32;

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

uint8_t extractAck(unsigned *pos, uint8_t *buf) {
  unsigned byte = *pos / 8;
  unsigned bit = *pos % 8;

  *pos += 3;

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

static void startRec(void) {
  seqStart = 0;
  seqSize = 0;
  progSize = 0;
  tdiPtr = recTdi;
  tmsPtr = recTms;
  readPtr = recRead;
  readFlagPtr = recReadFlag;
  initPosPtr = recInitPos;
  loopPosPtr = recLoopPos;
  ackPosPtr = recAckPos;
  endPosPtr = recEndPos;
  *tdiPtr = 0;
  *tmsPtr = 0;
  *readPtr = 0;
}

static void sendRec(void) {
  writeSeq(seqSize, recTdi, recTms);
}

static void readWriteRec(uint8_t *response_buf) {
  readWriteSeq(seqSize, recTdi, recTms, response_buf);
}

static void storeRec(void) {
  for(int i = 0; i < progSize; i++) {
    printf("Prog (%d) %d %d %d %d\n", recReadFlag[i], recInitPos[i], recLoopPos[i], recAckPos[i], recEndPos[i]);
  }
  unsigned words = seqSize / 8;
  if(seqSize % 8) words++;
  storeSeq(words, recTdi, recTms);
  storeProg(progSize, recReadFlag, recInitPos, recLoopPos, recAckPos, recEndPos);
}

static void recordSeq(unsigned size, uint8_t *tdiData, uint8_t *tmsData, uint8_t *readData) {
  for(int i = 0; i < size; i++) {
    unsigned sourceBit = i % 8;
    unsigned targetBit = seqSize % 8;

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

    seqSize++;
  }
}

static void recordCommand(uint8_t read, uint16_t initPos, uint16_t loopPos, uint16_t ackPos, uint16_t endPos) {
  *readFlagPtr++ = read;
  *initPosPtr++ = initPos;
  *loopPosPtr++ = loopPos;
  *ackPosPtr++ = ackPos;
  *endPosPtr++ = endPos;
  progSize++;
}

///////////////////////////////////////////////////////////////////////////////

static void fillByte() {
  unsigned bitsToAdd = 0;
  if(seqSize % 8) bitsToAdd = 8 - (seqSize % 8);
  if(bitsToAdd) {
    uint8_t tdi = 0;
    uint8_t tms = 0;
    uint8_t read = 0;
    recordSeq(bitsToAdd, &tdi, &tms, &read);
  }
}

static inline void gotoShiftIr(void) {
  uint8_t tms = 0x03;
  uint8_t tdi = 0;

  recordSeq(4, &tdi, &tms, NULL);
}

static inline void exitIrToShiftDr(void) {
  uint8_t tms = 0x03;
  uint8_t tdi = 0;

  recordSeq(4, &tdi, &tms, NULL);
}

static inline unsigned gotoShiftDr(void) {
  uint8_t tms = 0x01;
  uint8_t tdi = 0;

  recordSeq(3, &tdi, &tms, NULL);

  return 3;
}

static inline void exitIrToIdle(void) {
  uint8_t tms = 0x01;
  uint8_t tdi = 0;

  recordSeq(2, &tdi, &tms, NULL);
}

static inline unsigned exitDrToIdle(void) {
  uint8_t tms = 0x01;
  uint8_t tdi = 0;

  recordSeq(2, &tdi, &tms, NULL);

  return 2;
}

static uint32_t lastIr = -1;

static void writeIrInt(uint32_t idcode, uint32_t ir) {
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
      }
    }
    exitIrToIdle();
  }
}

static unsigned readWriteDrPre(uint32_t idcode, unsigned *postscan) {
  int total = 0;

  int devNum;
  for(devNum = 0; devNum < numDevices; devNum++) {
    struct JtagDevice *dev = &devices[devNum];
    if(dev->idcode == idcode) {
      break;
    }
  }
  if(devNum == numDevices) panic("Can't find device");

  total = gotoShiftDr();
  
  if(postscan) {
    *postscan = numDevices - devNum - 1;
  }

  if(devNum) {
    uint8_t tms[MAX_JTAG_DEVICES/8];
    uint8_t tdi[MAX_JTAG_DEVICES/8];

    memset(tms, 0, MAX_JTAG_DEVICES/8);
    memset(tdi, ~0, MAX_JTAG_DEVICES/8);

    recordSeq(devNum, tdi, tms, NULL);
    total += devNum;
  }

  return total;
}

static unsigned readWriteDrPost(unsigned postscan) {
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
  }

  return postscan + exitDrToIdle();
}

static void dpLowAccessFast(uint8_t RnW, uint16_t addr, uint32_t value, bool discard) {
  bool apndp = addr & APNDP;
  addr &= 0xff;

  uint16_t initPos = seqSize;

  writeIrInt(dpIdcode, apndp ? JTAG_IR_APACC : JTAG_IR_DPACC);

  fillByte();

  uint16_t loopPos = seqSize;

  unsigned postscan;
  readWriteDrPre(dpIdcode, &postscan);

  uint16_t ackPos = seqSize;

  { // addr + readbit
    uint8_t tdi = ((addr >> 1) & 0x06) | (RnW?1:0);
    uint8_t tms = 0;
    recordSeq(3, &tdi, &tms, NULL);
    fastBitsToRead += 3;
  }

  { // value
    uint8_t tms[4];
    memset(tms, 0, 4);

    if(!postscan) {
      tms[3] = 0x80;
    }
    recordSeq(32, (uint8_t*)&value, tms, NULL);

    if(!discard) fastBitsToRead += 32;
  }

  readWriteDrPost(postscan);

  fillByte();

  uint16_t endPos = seqSize;

  recordCommand(!discard, initPos, loopPos, ackPos, endPos);
}

static void dpReadFast(uint16_t addr) {
  dpLowAccessFast(LOW_READ, addr, 0, true);
  dpLowAccessFast(LOW_READ, DP_RDBUFF, 0, false);
}

static void dpWriteFast(uint16_t addr, uint32_t value) {
  dpLowAccessFast(LOW_WRITE, addr, value, true);
}

static void apWriteFast(unsigned apSel, uint16_t addr, uint32_t value) {
  dpWriteFast(DP_SELECT, ((uint32_t)apSel << 24)|(addr & 0xF0));
  dpWriteFast(addr, value);
}

static void coreReadRegFast(struct Core core, uint16_t reg) {
  uint32_t addr = core.baddr + 4*reg;
  apWriteFast(core.ap, AP_TAR, addr);
  dpReadFast(AP_DRW);
}

///////////////////////////////////////////////////////////////////////////////

void writeIr(uint32_t idcode, uint32_t ir) {
  startRec();
  writeIrInt(idcode, ir);
  sendRec();
}

void readWriteDr(uint32_t idcode, uint8_t *din, uint8_t *dout, int size) {
  startRec();
  unsigned postscan;
  readWriteDrPre(idcode, &postscan);
  sendRec();

  startRec();

  uint8_t tms[MAX_SEQ_SIZE];
  assert(size <= MAX_SEQ_SIZE);
  memset(tms, 0, size);

  if(!postscan) {
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
  bool apndp = addr & APNDP;
  addr &= 0xff;
  uint8_t ack = JTAG_ACK_WAIT;

  uint8_t response_buf[5];
  int tries = 1024;

  startRec();
  writeIrInt(dpIdcode, apndp ? JTAG_IR_APACC : JTAG_IR_DPACC);

  fillByte();

  unsigned ackPos;
  unsigned dataPos;

  while(tries > 0 && ack == JTAG_ACK_WAIT) {
    // create sequence

    unsigned postscan;
    readWriteDrPre(dpIdcode, &postscan);

    { // addr + readbit
      uint8_t tdi = ((addr >> 1) & 0x06) | (RnW?1:0);
      uint8_t tms = 0;
      uint8_t read = 0x07;
      ackPos = seqSize;
      recordSeq(3, &tdi, &tms, &read);
    }

    { // value
      uint8_t tms[4];
      uint8_t read[4];
      memset(tms, 0, 4);
      if(RnW) memset(read, 0xff, 4);
      else memset(read, 0, 4);

      if(!postscan) {
        tms[3] = 0x80;
      }

      dataPos = seqSize;
      recordSeq(32, (uint8_t*)&value, tms, read);
    }

    readWriteDrPost(postscan);

    fillByte();

    readWriteRec(response_buf);

    ack = extractAck(&ackPos, response_buf);

    tries--;
    startRec();
  }

  if(ack == JTAG_ACK_WAIT) {
    panic("Error: Invalid ACK (ack == JTAG_ACK_WAIT)\n");
  } else if(ack != JTAG_ACK_OK) {
    panic("Error: Invalid ACK (%x != JTAG_ACK_OK)\n", ack);
  }

  return extractWord(&dataPos, response_buf);
}

int getNumDevices(void) {
  uint8_t tdi[MAX_JTAG_DEVICES/8];
  uint8_t tms[MAX_JTAG_DEVICES/8];

  startRec();

  // set all devices in bypass by sending lots of 1s into the IR
  gotoShiftIr();
  memset(tdi, 0xff, MAX_JTAG_DEVICES/8);
  memset(tms, 0, MAX_JTAG_DEVICES/8);
  tms[(MAX_JTAG_DEVICES/8)-1] = 0x80;
  recordSeq(MAX_JTAG_DEVICES, tdi, tms, NULL);

  // flush DR with 0
  exitIrToShiftDr();
  memset(tdi, 0, MAX_JTAG_DEVICES/8);
  memset(tms, 0, MAX_JTAG_DEVICES/8);
  recordSeq(MAX_JTAG_DEVICES, tdi, tms, NULL);

  sendRec();

  // find numDevices by sending 1s until we get one back
  int num;

  for(num = 0; num < MAX_JTAG_DEVICES; num++) {
    uint8_t tms = 0;
    uint8_t tdi = 0x01;
    uint8_t tdo;

    readWriteSeq(1, &tdi, &tms, &tdo);
    if(tdo & 1) break;
  }

  gotoResetThenIdle();

  return num;
}

void getIdCodes(uint32_t *idcodes) {
  startRec();
  gotoShiftDr();
  sendRec();

  for(int i = 0; i < numDevices; i++) {
    uint8_t tms[4] = {0, 0, 0, 0};
    uint8_t tdi[4] = {0, 0, 0, 0};
    uint8_t tdo[4];

    readWriteSeq(32, tdi, tms, tdo);

    unsigned pos = 0;

    *idcodes++ = extractWord(&pos, tdo);
  }

  gotoResetThenIdle();
}

void gotoResetThenIdle(void) {
  uint8_t tms = 0x1f;
  uint8_t tdi = 0;

  writeSeq(6, &tdi, &tms);
}

#if 0

void coreReadPcsrInit(void) {
}

bool coreReadPcsrFast(uint64_t *pcs, bool *halted) {
  for(unsigned i = 0; (i < numCores) && (i < MAX_CORES); i++) {
    if(cores[i].enabled) {
      pcs[i] = coreReadPcsr(&cores[i]);
    }
  }

  if(cores[0].type == ARMV8A) {
    *halted = coreHalted(stopCore);
  } else {
    *halted = pcs[0] == 0xffffffff;
  }

  return true;
}

#else

void coreReadPcsrInit(void) {
  startRec();

  fastBitsToRead = 0;

  lastIr = -1;

  for(int i = 0; i < numCores; i++) {
    if(cores[i].type == ARMV7A) {
      coreReadRegFast(cores[i], ARMV7A_PCSR);

    } else if(cores[0].type == ARMV8A) {
      if(cores[i].enabled) {
        coreReadRegFast(cores[i], ARMV8A_PCSR_L);
        coreReadRegFast(cores[i], ARMV8A_PCSR_H);
      }
    }
  }

  if(cores[stopCore].type == ARMV8A) {
    coreReadRegFast(cores[stopCore], ARMV8A_PRSR);
  }

  //outputSpiCommands = true;
  storeRec();
  //outputSpiCommands = false;
}

bool coreReadPcsrFast(uint64_t *pcs, bool *halted) {
  *halted = false;
  uint32_t *words;

  executeSeq();

  words = getWords();

  unsigned core = 0;

  for(unsigned i = 0; i < numCores; i++) {
    if(cores[i].enabled) {
      if(cores[i].type == ARMV7A) {
        uint32_t dbgpcsr = *words++;

        if(i < MAX_CORES) {
          if(dbgpcsr == 0xffffffff) {
            pcs[i] = dbgpcsr;
          } else {
            pcs[i] = calcOffset(dbgpcsr);
          }
        }
      } else if(cores[0].type == ARMV8A) {
        uint32_t dbgpcsrLow = *words++;
        uint32_t dbgpcsrHigh = *words++;

        uint64_t dbgpcsr = ((uint64_t)dbgpcsrHigh << 32) | dbgpcsrLow;

        if(i < MAX_CORES) {
          if(dbgpcsr == 0xffffffff) {
            pcs[i] = 0;
          } else {
            pcs[i] = calcOffset(dbgpcsr);
          }
        }
      }

      core++;

    } else {
      if(i < MAX_CORES) {
        pcs[i] = 0;
      }
    }
  }

  if(cores[stopCore].type == ARMV8A) {
    uint32_t prsr = *words++;

    *halted = prsr & (1 << 4);

  } else if(cores[stopCore].type == ARMV7A) {
    if(pcs[stopCore] == 0xffffffff) *halted = true;
    else *halted = false;
  }

  return true;
}

#endif
